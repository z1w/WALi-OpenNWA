#ifndef wali_wpds_nwpds_NWPDS_GUARD
#define wali_wpds_nwpds_NWPDS_GUARD 1

/**
 * @author Prathmesh Prabhu
 */

// ::wali
#include "wali/Common.hpp"
// ::wali::wpds
#include "wali/wpds/WPDS.hpp"
#include "wali/wpds/Wrapper.hpp"
#include "wali/wpds/RuleFunctor.hpp"
// ::wali::wpds::fwpds
#include "wali/wpds/fwpds/FWPDS.hpp"
// ::wali::wfa
#include "wali/wfa/TransFunctor.hpp"
#include "wali/wfa/ITrans.hpp"
// ::std
#include <vector>

namespace wali
{
  namespace wfa
  {
    class WFA;
  }

  namespace wpds 
  {
    class Wrapper;

    namespace nwpds 
    {

      class NWPDS : public fwpds::FWPDS
      {
          /**
           * Newton Solver creates copies of some stack symbols in order to store values
           * computed in the last Newton iteration.
           * This keeps a map from the original Key to the Key_NEWTON_OLDVAL, generated within the
           * solver (call to poststar/prestar).
           **/
          typedef std::map< wali::Key,wali::Key > Key2KeyMap;

        public:
          /**
           * For parsing XML NWPDSs.
           * @shadow wali::wpds::nwpds::FWPDS::XMLTag
           */
          static const std::string XMLTag;

        public:
          NWPDS(bool dbg=true);
          NWPDS(ref_ptr<Wrapper> wrapper, bool dbg=true);
          NWPDS( const NWPDS& f );
          ~NWPDS();

          ///////////
          // pre*
          ///////////
          virtual void prestar( wfa::WFA const & input, wfa::WFA & output );

          virtual wfa::WFA prestar( wfa::WFA const & input) {
            wfa::WFA output;
            prestar(input,output);
            return output;
          }


        private:
          /**
           * Changes some rules of this PDS to actually solve the linearized prestar problem. 
           **/
          void prestarSetupPds();
          /**
           * After each linearized prestar, checks if the fa has changed, if yes, it updates the 
           * stored const values on the FA.
           **/
          bool prestarUpdateFa(wali::wfa::WFA& f);
          /**
           * Restores PDS to its former ruleset after prestar
           **/
          void prestarRestorePds();

          /**
           * Creates a map from new to old key if needed, and returns the old key for the new key.
           **/
          wali::Key getOldKey(wali::Key newKey);

          //void poststarSetupPds();
          //bool poststarUpdateFa();
        public:

          class UpdateFaFunctor : public wali::wfa::TransFunctor
          {
            public:
              UpdateFaFunctor(wali::wfa::WFA& fa, Key2KeyMap& new2OldMap); 
              virtual void operator()(wali::wfa::ITrans* t);
              bool updated() { return changed; }
            private:
              wali::wfa::WFA& fa;
              const Key2KeyMap& new2OldMap;
              bool changed;
          };


        private:

          Key2KeyMap var2ConstMap;

          /**
           * We modify the pds to linearize the equations to be solved.
           * This vector stores the rules that must be restored after we're done.
           **/
          std::vector<rule_t> savedRules;

          /**
           * Facilitate printing by creating a NWPDS with dbg set to true.
           **/
          bool dbg;
      };

    /**
     * 
     * @class Delta2Rules
     *
     * Gets all delta_2 rules from the wpds. Holds on to const rule_t
     * so that it can be cast down to the correct ERule/Rule type.
     */
    class Delta2Rules: public RuleFunctor
    {
      public:
        std::vector< rule_t > rules;
        Delta2Rules();
        virtual ~Delta2Rules();
        virtual void operator() (rule_t & r);
    };



    } //namespace nwpds
  } //namespace wpds
} //namespae wali
#endif //wali_wpds_newpds_NWPDS_GUARD