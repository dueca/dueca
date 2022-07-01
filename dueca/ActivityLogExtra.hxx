/* ------------------------------------------------------------------   */
/*      item            : ActivityLogExtra.hxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional header code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** Pointer to the list of ActivityBit objects */
ActivityBitPtr bit_list;

/** Pointer to the last bit in the list. */
ActivityBitPtr bit_tail;

/** Number of ActivityBit objects in this log. Each ActivityBit
    represents an elementary action, start of an Activity, suspend of
    the ActivityManager or blocking/resuming with blocking IO */
uint16_t no_of_bits;

/** Add an activity bit. */
inline void appendActivityBit(ActivityBitPtr b)
{if (no_of_bits < 0xffff) {bit_tail = bit_tail->setNext(b); no_of_bits++;}}

/** Constructor with a sinble (starting) activity bit */
ActivityLog(const uint8_t& node_id,
            const uint8_t& manager_number,
            const TimeTickType& base_tick,
            const double& fraction_mult,
            const ActivityBitPtr& bit_list);
