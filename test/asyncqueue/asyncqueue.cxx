#include <AsyncQueueMT.hxx>
#include <iostream>
#include <pthread.h>
#include <cassert>

using namespace std;
using namespace dueca;

const int NTHREADS=20;
const int NMESG = 1000000;

#define DEB(A)
#ifndef DEB
#define DEB(A) cout << A << endl;
#endif
#define DEBR(A)
#ifndef DEBR
#define DEBR(A) cout << A << endl;
#endif

// type to send around
struct MyData
{
  int threadno;
  int messageno;
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
  AsyncQueueMT<MyData> *queue;

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
    {
      AsyncQueueWriter<MyData> w( *(res->queue) );
      w.data().threadno = res->myid;
      w.data().messageno = msg;
      DEB("wrote at " << hex << reinterpret_cast<void*>(&w.data()) << dec);
    }

    if (msg % (11+res->myid) == 0) {
      struct timespec twait = {0, 100};
      nanosleep(&twait, NULL);
    }
  }

  return NULL;
}

int main()
{
  AsyncQueueMT<MyData> tq(NTHREADS*NMESG);
  assert(!tq.notEmpty());
  {
    AsyncQueueWriter<MyData> w(tq);
    w.data().threadno = 10;
    w.data().messageno = 11;
  }
  assert(tq.notEmpty());
  {
    AsyncQueueReader<MyData> r(tq);
    assert(r.data().threadno == 10);
    assert(r.data().messageno == 11);
  }
  assert(!tq.notEmpty());

  AsyncQueueMT<MyData> common_queue;

  assert(!common_queue.notEmpty());


  pthread_t threads[NTHREADS];
  Result rest[NTHREADS];
  Result res;

  for (int t = NTHREADS; t--; ) {
    rest[t] = Result();
    rest[t].myid = t;

  rest[t].queue = &common_queue;
    pthread_create(&threads[t], NULL, threadwork, &rest[t]);
  }

  for (int nmesg = NTHREADS*NMESG; nmesg > 0; ) {
    AsyncQueueReader<MyData> r(common_queue);
    if (r.valid()) {
      DEBR("FN " << " FROM " << r.data().threadno
          << " MSG " << r.data().messageno<< " OLD " <<
            res.messageno[r.data().threadno]);

      if (res.messageno[r.data().threadno] >= r.data().messageno) {
        cout << "disorder " << endl;
      }
      res.msgreceived[r.data().threadno]++;
      res.messageno[r.data().threadno] = r.data().messageno;
      nmesg--;
    }
  }

  for (int t = NTHREADS; t--; ) {
    pthread_join(threads[t], NULL);
  }

  cout << res;

  cout << common_queue.size() << endl;
  assert(!common_queue.notEmpty());

  return 0;
}
