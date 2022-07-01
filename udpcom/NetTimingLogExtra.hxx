/* ------------------------------------------------------------------   */
/*      item            : NetTimingLogExtra.hxx
        made by         : Rene van Paassen
        date            : 170205
        category        : header file
        description     :
        changes         : 170205 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef NetTimingLogExtra_hxx
#define NetTimingLogExtra_hxx

/** Enter a datapoint in the histogram log.

    @param meastime  Measured time
    @param cycletime Cycle time */
void histoLog(unsigned meastime, unsigned cycletime);

/** fraction of regular timing

    @param idx       Histogram bin selection
*/
float histTime(unsigned idx) const;

/** Print a header to a file */
static void printhead(std::ostream& os, const std::string& label);

/** Print a line in a file with compact timinglog data */
void printline(std::ostream& s, TimeTickType tick) const;

#endif
