/* ------------------------------------------------------------------   */
/*      item            : LogLevelExtra.hxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional header code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

namespace dueca {
LogLevel LogLevel_from_text(const char* txt);

char LogLevel_to_letter(LogLevel lvl);
}