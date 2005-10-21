#ifndef wali_DEFAULT_WORKLIST_GUARD
#define wali_DEFAULT_WORKLIST_GUARD 1

/*!
 * @author Nick Kidd
 */

#include "wali/Common.hpp"
#include "wali/Worklist.hpp"
#include <list>

namespace wali
{
    namespace wfa {
        class Trans;
    }

    /*! @class DefaultWorklist
     *
     * The default DefaultWorklist acts as a Stack and uses
     * std::list to hold items.
     */

    class DefaultWorklist : public ::wali::Worklist
    {
        public:

            DefaultWorklist();

            virtual ~DefaultWorklist();

            /*!
             * put
             *
             */
            virtual void put( wfa::Trans *item );

            /*!
             * get
             *
             * Return an item from the worklist.
             * Returns NULL if the DefaultWorklist is empty.
             * In the future it may throw an exception
             *
             * @return wfa::Trans *
             */
            virtual wfa::Trans * get();

            /*!
             * emtpy
             *
             * @return true if the DefaultWorklist is empty
             */
            virtual bool empty() const;

            /*!
             * clear
             *
             * Remove and unmark each item in this worklist.
             */
            virtual void clear();

        protected:
            std::list< wfa::Trans * > wl; //!< The default worklist data structure

    }; // class DefaultWorklist

} // namespace wali

#endif // wali_DEFAULT_WORKLIST_GUARD

/* Yo, Emacs!
   ;;; Local Variables: ***
   ;;; tab-width: 4 ***
   ;;; End: ***
 */
