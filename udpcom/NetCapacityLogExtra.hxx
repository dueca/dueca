/* ------------------------------------------------------------------   */
/*      item            : NetCapacityLogExtra.hxx
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

#ifndef NetCapacityLogExtra_hxx
#define NetCapacityLogExtra_hxx

/** custom constructor added */
NetCapacityLog(const uint16_t& node_id);

/** Enter a datapoint in the histogram log.

    @param regular  Regular bytes in message
    @param fill     Fill bytes in message
    @param capacity Max size of message */
void histoLog(unsigned regular, unsigned fill, unsigned capacity);

/** fraction of regular fill */
float histRegular(unsigned idx) const;

/** fraction of total fill */
float histTotal(unsigned idx) const;

/** Print a header to a file */
static void printhead(std::ostream& os, const std::string& label);

/** Print a line in a file with compact net load data */
void printline(std::ostream& s, TimeTickType tick) const;

#endif
