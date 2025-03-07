/* ------------------------------------------------------------------   */
/*      item            : LogLevelExtra.cxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional body code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110
#define __CUSTOM_COMPATLEVEL_111

LogLevel LogLevel_from_text(const char* text)
{
  switch(text[0]) {
    case 'd': return LogLevel::Debug;
    case 'i': return LogLevel::Info;
    case 'w': return LogLevel::Warning;
    case 'e': return LogLevel::Error;
    default:
    return LogLevel::Invalid;
  }
}

char LogLevel_to_letter(LogLevel lvl)
{
  switch(lvl) {
    case LogLevel::Debug: return 'd';
    case LogLevel::Info: return 'i';
    case LogLevel::Warning: return 'w';
    case LogLevel::Error: return 'e';
    default:
    return 'x';
  }
}