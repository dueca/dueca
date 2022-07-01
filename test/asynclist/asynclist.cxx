#include <AsyncList.hxx>
#include <iostream>
#include <pthread.h>
#include <cassert>
#include <array>

using namespace std;
using namespace dueca;

const int NTHREADS=1;
const int NMESG = 100;

//#define DEB(A)
#ifndef DEB
#define DEB(A) cout << A << endl;
#endif
//#define DEBR(A)
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
  AsyncList<MyData> *queue;
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
      AsyncListWriter<MyData> w( *(res->queue) );
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
  AsyncList<MyData> tq(5);
  assert(!tq.notEmpty());
  {
    AsyncListWriter<MyData> w(tq);
    w.data().threadno = 10;
    w.data().messageno = 11;
  }
  assert(tq.notEmpty());
  assert(tq.front().threadno == 10);
  assert(tq.front().messageno == 11);
  tq.pop();
  assert(!tq.notEmpty());

  std::array<AsyncList<MyData>,NTHREADS> common_queue;

  assert(!common_queue[0].notEmpty());

  pthread_t threads[NTHREADS];
  Result rest[NTHREADS];
  Result res;

  for (int t = NTHREADS; t--; ) {
    rest[t] = Result();
    rest[t].myid = t;

    rest[t].queue = &common_queue[t];
    pthread_create(&threads[t], NULL, threadwork, &rest[t]);
  }

  for (int nmesg = NTHREADS*NMESG; nmesg > 0; ) {
    for (int t = NTHREADS; t--; ) {
      if (common_queue[t].notEmpty()) {

        DEBR("FN " << " FROM " << common_queue[t].front().threadno
             << " MSG " << common_queue[t].front().messageno<< " OLD " <<
             res.messageno[common_queue[t].front().threadno]);

        res.msgreceived[common_queue[t].front().threadno]++;
        res.messageno[common_queue[t].front().threadno] = common_queue[t].front().messageno;
        common_queue[t].pop();
        nmesg--;
      }
    }
  }

  for (int t = NTHREADS; t--; ) {
    pthread_join(threads[t], NULL);
    while (common_queue[t].notEmpty()) {
      DEBR("extra message q=" << t << "=" << common_queue[t].front().threadno
           << " msg="  << common_queue[t].front().messageno);
      common_queue[t].pop();
    }
  }

  cout << res;
  for (int t = NTHREADS; t--; ) {
    cout << "thread " << t << " size " << common_queue[t].size() << endl;
    assert(common_queue[t].isEmpty());
  }
  return 0;
}
