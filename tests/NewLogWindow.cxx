#include <LogViewGui.hxx>
#include <dueca_ns.h>
#include <gtk/gtk.h>

DUECA_NS_START

const LogCategory& cat_one()
{
  static const LogCategory cat("ONE", "Configuration and scripting");
  return cat;
}
const LogCategory& cat_two()
{
  static const LogCategory cat("TWO", "Configuration and scripting");
  return cat;
}
const LogCategory& cat_tre()
{
  static const LogCategory cat("TRE", "Configuration and scripting");
  return cat;
}

class LogView
{
  LogViewGui* gui;
public:

  LogView() :
    gui(new LogViewGui(this, 4))
  {
    gui->open();

    gui->appendLogCategory(cat_one());
    gui->appendLogCategory(cat_two());
    gui->appendLogCategory(cat_tre());

  }
};

DUECA_NS_END

int main(int argc, char* argv[])
{
  gtk_init(&argc, &argv);

  dueca::LogView mytester;


  gtk_main();
}
