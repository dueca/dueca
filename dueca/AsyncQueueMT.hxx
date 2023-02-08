/* ------------------------------------------------------------------   */
/*      item            : AsyncQueueMT.hxx
        made by         : Rene van Paassen
        date            : 131227
        category        : header file
        description     : Asynchronous, multi-threaded queue,
                          for design sketch see doc/asyncqueuedesign.xoj
                          following
                          http://nedko.arnaudov.name/soft/L17_Fober.pdf
                          this is also in boost use that!
                          other option:
http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html
                          16 byte CAS:
http://blog.lse.epita.fr/articles/42-implementing-generic-double-word-compare-and-swap-.html

        changes         : 131227 split off from AsyncQueue, to make a
                          multi-thread version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef AsyncQueueMT_hxx
#define AsyncQueueMT_hxx

#include <sys/types.h>
#include <string>
#include <iostream>
#include "LockFreeLIFO.hxx"
#include "ListElementAllocator.hxx"
#include <pthread.h>

#define USE_SPARES

#include <dueca_ns.h>
DUECA_NS_START

template <class T, class Alloc> class AsyncQueueWriter;
template <class T, class Alloc> class AsyncQueueReader;
template<class T, class Alloc> class AsyncQueueMT;
template <class T, class Alloc>
typename Alloc::element_ptr get_list_spare(AsyncQueueMT<T,Alloc>& list);
template <class T, class Alloc>
void write_list_back(AsyncQueueMT<T,Alloc>& list,
                     typename Alloc::element_ptr elt);
template <class T, class Alloc>
void return_list_elt(AsyncQueueMT<T,Alloc>& list,
                     typename Alloc::element_ptr elt);


/** This implements a singly linked fifo queue, which uses a sentinel
    technique and the atomic_swap64 call for implementing non-blocking
    transfer of commands/data from multiple threads to a single
    reading thread. By design, this is only thread-safe for a single
    reader and multiple writers, and only for the inserting and
    extracting.

    Use the dueca::AsyncQueueWriter and dueca::AsyncQueueReader
    helpers for inserting and extracting data.

    Accessing members in the list through the front() and back()
    functions is possible, but not locked in any way. As long as the
    single reading thread does it, using front() will be safe. Using
    back() will only be safe if there is no reading/pop() action, and
    no other threads write.
 */
template<class T, class Alloc=ListElementAllocator<T> >
class AsyncQueueMT
{
  /** only access to writing data on the queue */
  friend class AsyncQueueWriter<T,Alloc>;

  /** only point of access for reading data from the queue */
  friend class AsyncQueueReader<T,Alloc>;

  /** friend function for accessing an element from the spare stack */
  friend typename Alloc::element_ptr
  get_list_spare<T,Alloc>(AsyncQueueMT<T,Alloc>& list);

  /** write back a previously obtained spare */
  friend void write_list_back<T,Alloc>(AsyncQueueMT<T,Alloc>& list,
                                       typename Alloc::element_ptr elt);

  /** return a previously obtained spare to the spares */
  friend void return_list_elt<T,Alloc>(AsyncQueueMT<T,Alloc>& list,
                                       typename Alloc::element_ptr elt);

  /** Shorthand for a single element in the linked list */
  typedef typename Alloc::element_type element_type;

  /** Shorthand for a pointer to a list element */
  typedef typename Alloc::element_ptr element_ptr;

  /** Atomic version of the pointer */
  typedef typename atom_type<element_ptr>::type atomic_ptr;

  /** A name for the asyncqueue, optional, but useful. */
  const std::string name;

  /** The head of the list of spares. */
  LockFreeLIFO<element_type> spares;

  /** The head of the list of data. */
  atomic_ptr data_head;

  /** The tail of the list of data. */
  //volatile typename atom_type<Element<T>*>::type data_tail;
  atomic_ptr data_tail;

  pthread_mutex_t guard;

  /** A count of the number of entered elements. */
  typename atom_type<uint64_t>::type entered;

  /** And a count of the number of removed elemements. */
  uint64_t removed;


public:
  /** Allocator used. Normally (default allocator) not modifyable, but
      custom allocators may be tuned. */
  Alloc allocator;
public:
  /** Type of the objects in the queue */
  typedef T                                    value_type;

  /** List initialisation, special case for when init should be later */
  void init_list(int size)
  {
    if (atomic_access(data_head) != NULL) return;
    data_head = getSpare();
    data_tail = atomic_access(data_head);
  }

  /** Constructor. */
  AsyncQueueMT(int size = 10, const char* name = "anon AsyncQueueMT") :
    name(name),
    spares(),
    data_head(NULL),
    data_tail(NULL),
    entered(0),
    removed(0)
  {
    if (size) init_list(size);
  }

  /** Destructor. 

      @todo This apparently does not clean up all points? */
  ~AsyncQueueMT()
  {
    element_ptr to_delete = spares.pop();
    while (to_delete) {
      delete to_delete;
      to_delete = spares.pop();
    }
    to_delete = atomic_access(data_head);
    while ( to_delete) {
      data_head = atomic_access(data_head)->next;
      delete to_delete;
      to_delete = atomic_access(data_head);
    }
  }

  /** Returns true if there is more than the sentinel. */
  inline bool notEmpty() const {return atomic_access(data_head)->next != NULL;}

  /** Returns true if there is no data to be read. */
  inline bool isEmpty() const {return atomic_access(data_head)->next == NULL;}

private:
  /** Verify ABA problem??? */

  /** Obtain a spare or new object to write to */
  element_ptr getSpare()
  {

    /* ABA analysis: spares is written by returnSpare, and by this
       function.  For a spare to re-appear on the spares stack, it
       would have to be used by another thread and processed by the
       consuming thread. It would be a problem if its next pointer
       had changed to NULL, but this can only happen for the
       sentinel, which stays in the spares stack and is not used. OK. */
    element_ptr currentspare = spares.pop();

    if (currentspare != NULL) {

      // use the in-place constructor
      return allocator(reinterpret_cast<void*>(currentspare));
    }

    return allocator(); //new Element<T>();
  }

  /** queue new data into the Queue */
  void writeTail(element_ptr newtail)
  {
    /* ABA analysis: ?? */
    /* With poor ABA, some other thread might have updated the next
       pointer of data_tail/tmptail too. This means that their newtail
       is not visible. */
    element_ptr tmptail = atomic_access(data_tail);

    while (!atomic_swap64(&data_tail, tmptail, newtail)) {
      tmptail = data_tail;
    }
    /* this works because the compare and swap has a memory barrier? */
    tmptail->next = newtail;
    atomic_increment64(entered);
  }

  /** get the next element for reading. The one at the head of the
      queue is already consumed or being consumed by another. */
  element_ptr getHead()
  {
    /* ABA analysis NA; do this in one thread only */
    element_ptr tmphead;
    do {
      tmphead = atomic_access(data_head);
      if (tmphead->next == NULL) return NULL;
    } while (!atomic_swap64(&data_head, tmphead, tmphead->next));
    element_ptr data = tmphead->next;
    returnSpare(tmphead);
    removed++;
    return data;
  }

  /** return a processed element to the spares stack */
  void returnSpare(element_ptr spare)
  {
    spares.push(spare);
  }
public:

  /** Peek at data without removing */
  inline const T& front() const { return atomic_access(data_head)->next->data; }

  /** Peek at data without removing */
  inline const T& back() const { return atomic_access(data_tail)->data; }

  /** Access data */
  inline T& front() { return atomic_access(data_head)->next->data; }

  /** Access data */
  inline T& back() { return atomic_access(data_tail)->data; }


  /** Remove element at head */
  inline void pop()
  {
    element_ptr tmphead;
    do {
      tmphead = data_head;
      if (tmphead->next == NULL) return;
    } while (!atomic_swap64(&data_head, tmphead, tmphead->next));
    returnSpare(tmphead);
    removed++;
  }

  /** Return the number of elements in the list. */
  inline unsigned int size() {return atomic_access(entered) - removed;}

  /** Push back, with a writer, convenience method, involves an assigment
      of the to-be pushed object */
  inline void push_back(const T& data)
  {
    AsyncQueueWriter<T,Alloc> wr(*this);
    wr.data() = data;
  }

  /** Construct an element at the end of the asynchronous list.
      @param args   Arguments for constructing the element.
  */
  template<class... Args>
  inline void emplace_back(Args&&... args)
  {
    element_type *_data = getSpare();
    ::new (&(_data->data)) T(std::forward<Args>(args)...);
    writeTail(_data);
  }
};

/** Lightweight helper object for writing an async queue

    Create on stack, destructor signals end of writing and passes to
    next data.
 */
template <class T, class Alloc=ListElementAllocator<T> >
class AsyncQueueWriter
{
  /** Pointer to the queue to be written. */
  AsyncQueueMT<T,Alloc>                    *_queue;

  /** Pointer to the written data point */
  typename AsyncQueueMT<T,Alloc>::element_type   *_data;

public:
  /** Constructor.
   */
  AsyncQueueWriter(AsyncQueueMT<T,Alloc>& q) :
    _queue(&q),
    _data(q.getSpare())
  { }

   /** Destructor.
    */
  ~AsyncQueueWriter()
  { _queue->writeTail(_data); }

  /** Access a reference to the data */
  T& data() { return _data->data; }
};

/** Lightweight helper object for reading an async queue

    Create on stack, destructor signals end of reading and passes to
    next data.
 */
template <typename T, class Alloc=ListElementAllocator<T> >
class AsyncQueueReader
{
  /** Pointer to the queue to be read. */
  AsyncQueueMT<T,Alloc>                          *_queue;

  /** Pointer to the read data point */
  typename AsyncQueueMT<T,Alloc>::element_type   *_data;

public:
  /** Constructor.
   */
  AsyncQueueReader(AsyncQueueMT<T,Alloc>& q) :
    _queue(&q),
    _data(q.getHead())
  { }

   /** Destructor.
    */
  ~AsyncQueueReader()
  { }

  /** Test whether data is present */
  inline bool valid() const {return _data != NULL;}

  /** Access a reference to the data */
  const T& data() { return _data->data; }
};


/** Retrieve a loose/spare element from the list.

    Note that you are responsible for returning this element with
    write_list_back or return_list_spare

    @tparam T     Type of object
    @tparam Alloc Memory allocator/handler for this list

    @param list   List object
*/
template <class T, class Alloc>
inline typename Alloc::element_ptr
get_list_spare(AsyncQueueMT<T,Alloc>& list)
{
  return list.getSpare();
}

/** Append a previously retrieved element at the back of the list.

    Note that you are responsible for obtaining this element with
    get_list_spare

    @tparam T     Type of object.
    @tparam Alloc Memory allocator/handler for this list.
    @param list   List object.
    @param elt    Element to append.
*/
template <class T, class Alloc>
inline void write_list_back(AsyncQueueMT<T,Alloc>& list,
                     typename Alloc::element_ptr elt)
{
  list.writeTail(elt);
}

/** Recycle a previously retrieved element from the list

    Note that you are responsible for obtaining this element with
    get_list_spare. Note: either recycle or append, do not do both!

    @tparam T     Type of object.
    @tparam Alloc Memory allocator/handler for this list.
    @param list   List object.
    @param elt    Element to recycle.
*/
template <class T, class Alloc>
inline void return_list_elt(AsyncQueueMT<T,Alloc>& list,
                            typename Alloc::element_ptr elt)
{
  list.returnSpare(elt);
}



DUECA_NS_END
#endif
