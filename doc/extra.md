#  Additional utilities {#extra}

As simulations are developed with DUECA and DUSIME, additional
tools for simulations will be developed, and once these tools are made
re-usable, they will be included in the DUECA distribution. Note that
these tools have nothing (or maybe little) to do with DUECA itself,
they can be seen as separate from the communication and activation
tools offered by DUECA. This documentation section is a sort of
gathering of such tools.

##  Facilitiating OpenGL drawing {#extra_opengl}

To facilitate OpenGL drawing, a class dueca::DuecaGLWindow has been
developed. The common use for modules that want to have GL output, for
example a module that provides an outside visual or instrument
display, would be to create a class (normally not the module class
itself, but if you want to experiment with multiple inheritance, you
might consider it) that derives from %DuecaGLWindow. This derived
class should implement at least an implementation for the virtual
function dueca::DuecaGLWindow::display. This implementation should use
opengl commands to draw the image. Further details are in the
%DuecaGLWindow documentation.

You can either run the GL window in priority level 0. If you want to
run the GL window in another priority level, you must use the
GlutSweeper. This is a dueca module that takes care of glut
initialisation, and ensures that an update is called after you
requested a redraw for your window(s).

##  Random number generator {#extra_random}

The function dueca::randNormal() provides a random generator for
random numbers with a normal (Gaussian) distribution and a standard
deviation of 1.

##  Gtk+ experiment windows. {#extra_gtk}

The class dueca::GtkGladeWindow can create an experiment window
directly from a file created with glade, a GUI creation tool. By using
a table with callback functions, of type dueca::GladeCallbackTable,
you can easily connect the actions from the widgets to a C++ class
handling the interface.

##  Interpolation facilities {#extra_interpolation}

Linear table interpolation for tables up to four dimensions is
available from the dueca::Interpolator1, dueca::Interpolator2,
dueca::Interpolator3 and dueca::Interpolator4 classes.

##  Rigid body motions {#extra_rigidbody}

A RigidBody class provides the basis for implementing the dynamics of
a single rigid body. One can make a derived class, in which the forces
and gravitational pull on the body are defined. Optionally this class
can extend the state vector of the RigidBody parent. With integration
routines dueca::integrate_euler() or dueca::integrate_rungekutta() ,
the combination may be integrated numerically. These integration
function templates can also be used separately.

##  Linear system simulation {#extra_linearsystem}

dueca::LinearSystem is a class for calculating linear system
responses. It accepts the linear system as a continuous-time transfer
function or state-space system. The variant dueca::LimitedLinearSystem
implements a linear system with limiter.

##  Hardware input and output {#extra_io}

For converting and calibrating signals that go to and from hardware
(AD and DA converters, and the like), and possibly for other things as
well, a set of classes has been defined:

- dueca::SingletonPointer, can be used to ensure only one instance of a
  hardware controlling class is created.

- dueca::OutputCalibrator, for calculating integer output signals from a
  floating point value put in.

- dueca::InputCalibrator, for doing the reverse.

- dueca::Polynomial and dueca::PolynomialN, for implementing a
  calibration polynomial.

- dueca::Steps and dueca::StepsN, for implementing a stepping
  output, e.g. for reading flap selections in complete increments.

- dueca::Circular and dueca::CircularWithPoly, for implementing a
  conversion for circular (e.g. synchro) values.

- dueca::Inverse, for implementing an inverting (division) conversion, e.g. from interval time to rate.

- dueca::InputRatioCalibrator, for calculating an value as a ratio with, e.g., a reference voltage.

##  Finding files {#extra_findfiles}

The class dueca::FindFiles creates a vector of file names matching a glob patter.

##  Creating a new unique file {#extra_uniquefile}

The class dueca::UniqueFile creates a numbered, unique file name based on a
specified format string.

##  Conglomerate/distributed factory {#extra_factory}

The templated class dueca::ConglomerateFactory implements an extensible
factory pattern. This is used in for example the WorldView project to create
families of graphical objects.

##  Axis transformations {#extra_axis}

The file AxisTransforms.hxx contains objects for common axis transformations.

