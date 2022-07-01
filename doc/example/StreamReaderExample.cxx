/* Here is a piece of your doStep() method, in which you would like
   to read data from a stream channel: */
MyClass::doStep(const TimeSpec& ts)
{
  switch(getAndCheckState(ts)) {
  case HoldCurrent:
    /* stuff to do in HoldCurrent. */
    break;

  case Advance:

    /* get the latest data from the input channel*/
    try {
      /* The constructor takes the access token "input", and tries to
         obtain read access. This may throw a NoDataAvailable event,
         so it is in a try block. */
      StreamReaderLatest<MyInput> read_input(input);

      /* At what time was this written? */
      cout << "Input was written for " << read_input.timeSpec()
           << endl;

      /* And what was in there, suppossing objects of type MyInput
         have members variable1 and variable2? */
      cout << "Input data, variable1=" << read_input.data().variable1
           << " variable2="  << read_input.data().variable2 << endl;

      /* as the read_input variable goes out of scope, its destructor
         is called, and access to the channel is released again. */
    }
    catch (Exception& e) {
      /* Caught an exception, probably there is no data yet in the
         channel. */
      cout << "In MyClass::doStep,\n"
           << "Exception 1 accessing input channel\n"
           << e.what() << endl;
    }


    /* Another example, get the input corresponding to the time
       specification of this activity. Here you have to supply the
       time specification. */
    try {
      StreamReader<AnotherInput> read_other(other, ts);

      /* We know the time, that is the activity's time spec. What was
         the data? */
      cout << "other data " << read_other.data() << endl;

      /* Again, as the stream reader goes out of scope, it is
         destructed, and read access is released. */
    }
    catch (Exception& e) {
      /* Caught an exception, probably there is no data yet in the
         channel. */
      cout << "In MyClass::doStep,\n"
           << "Exception 2 accessing input channel\n"
           << e.what() << endl;
    }
    break;
  }
}
