#include <ArenaPool.hxx>
#include <Arena.hxx>
#include <iostream>
#include <pthread.h>
#include <cassert>

using namespace std;
using namespace dueca;

#define USE_ARENA

const int NTHREADS=7;
const int NMESG = 1000000;

#define DEB(A)
#ifndef DEB
#define DEB(A) cout << A << endl;
#endif

// type to allocate
struct MyData
{
  int64_t threadno;
  int64_t messageno;

#ifdef USE_ARENA
  /** new operator "new", which places objects not on a
      heap, but in one of the memory arenas. This may prevent
      problems with asymmetric allocation */
  static void* operator new(size_t size)
  {
    assert(size == sizeof(MyData));
    static Arena* my_arena = arena_pool.findArena(size);
    return my_arena->alloc(size);
  }
  /** new operator "delete", to go with the new version
      of operator new. */
  static void operator delete(void* p) throw()
  {
    static Arena* my_arena = arena_pool.findArena(sizeof(MyData));
    my_arena->free(p);
  }
#endif
};

struct Result
{
  Result() :
    myid(0)
  {
    for (int ii = NTHREADS; ii--; ) {
      msgreceived[ii] = 0;
      messageno[ii] = -1;
    }
  }
  volatile int msgreceived[NTHREADS];
  volatile int messageno[NTHREADS];
  int myid;
};

Result operator+(const Result& i, const Result& o)
{
  Result res;
  for (int ii = NTHREADS; ii--; ) {
    res.msgreceived[ii] = i.msgreceived[ii] + o.msgreceived[ii];
    res.messageno[ii] = max(i.messageno[ii], o.messageno[ii]);
  }
  return res;
}


ostream& operator<< (ostream& s, const Result& r)
{
  s << "thread " << r.myid << endl;
  for (int t = 0; t < NTHREADS; t++) {
    s << "  from " << t << " n=" << r.msgreceived[t]
      << " latest=" << r.messageno[t] << endl;
  }
  return s;
}

void *threadwork(void* r)
{
  Result* res = reinterpret_cast<Result*>(r);

  cout << "Entered thread " << res->myid << endl;

  for (int msg = 0; msg < NMESG; msg++) {
    MyData* n = new MyData();
    if (msg % (11+res->myid) == 0) {
      struct timespec twait = {0, 100};
      nanosleep(&twait, NULL);
    }
    delete n;
  }

  return NULL;
}

int main()
{
  pthread_t threads[NTHREADS];
  Result rest[NTHREADS];
  Result res;

  for (int t = NTHREADS; t--; ) {
    rest[t] = Result();
    rest[t].myid = t;
    pthread_create(&threads[t], NULL, threadwork, &rest[t]);
  }

  for (int t = NTHREADS; t--; ) {
    pthread_join(threads[t], NULL);
  }

  cout << res;

  Arena *arena= ArenaPool::single().findArena(sizeof(MyData));

  cout << *arena << endl;


  return 0;
}
