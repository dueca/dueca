/* ------------------------------------------------------------------   */
/*      item            : ChannelWriteInfoExtra.hxx
        made by         : Rene' van Paassen
        date            : 180329
        category        : additional header code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2018 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** Print a header for a file with compact readinfo data */
static void printhead(std::ostream& s);

/** Print a line in a file with compact readinfo data */
void printline(std::ostream& s) const;
