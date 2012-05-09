#ifndef wali_genereate_Random_FWPDS_guard
#define wali_genereate_Random_FWPDS_guard 1

// ::wali::wpds::fwpds
#include "wali/wpds/fwpds/FWPDS.hpp"

// ::wali
#include "wali/Key.hpp"
#include "wali/SemElem.hpp"
#include "wali/ref_ptr.hpp"
#include "wali/Countable.hpp"

// ::std
#include <string>
#include <vector>
#include <fstream>

namespace wali 
{
  namespace wpds
  {

    class RandomPdsGen;
    typedef wali::ref_ptr< RandomPdsGen > random_pdsgen_t;

    class RandomPdsGen : public Countable
    {
      public:
        class WtGen : public Countable
        {
          public:
            /**
             * Extend this class and implement this function so I can get random weights.
             **/
            WtGen() : Countable() {}
            virtual sem_elem_t operator () () = 0;
            virtual ~WtGen() {}
        };
        typedef wali::ref_ptr< WtGen > wtgen_t;

        /**
         * Used to return the important names to the function  calling randfwpds(...)
         */
        struct Names
        {
          wali::Key pdsState;
          wali::Key *entries;
          wali::Key *exits;
          std::vector< wali::Key > errs;
          Names();
          ~Names();
        };

        /**
         * Generate a random FWPDS with the configuration given by conf
         * @detail ...
         * @see struct Conf
         **/
        RandomPdsGen(
            wtgen_t randomWt,
            int numprocs = 100,
            int numcalls = 100,
            int numnodes = 400,
            int numsplits = 100,
            int numerrs = 0,
            double pCall = 0.45, 
            double pSplit = 0.45
            );
        ~RandomPdsGen();
        void get(wali::wpds::fwpds::FWPDS& pds, Names& names, std::ostream * o = NULL);

      private:
        // /Forward declaration of functions.
        void genproc(
            wali::wpds::fwpds::FWPDS& pds, 
            int procnum, 
            wali::Key curKey,
            int remNodes, 
            int remSplits, 
            int remCalls, 
            std::ostream *o, 
            int tabstop
            );
        wali::Key genblock(
            wali::wpds::fwpds::FWPDS& pds, 
            wali::Key curhead, 
            wali::Key curKey,
            int remNodes, 
            int remSplits, 
            int remCalls, 
            std::ostream *o, 
            int tabstop
            );

      private:
        //Configuration of the Generator as set by the constructor
        //! pointer to a function that generates random weights
        wtgen_t randomWt;
        //! The number of procedures to be generated
        int numprocs;
        //! The number of callsites to be generated
        int numcalls;
        //! The number of cfg nodes to be generated [loosely followed]
        int numnodes;
        //! The number of two-way splits to be generated.
        int numsplits;
        //! Number of error points to be generated
        int numerrs;
        //! Probability with wich a call is generated
        double pCall;
        //! Probability with wich a split is generated
        double pSplit;

        // Data Used during the generation of a new randomd PDS
        // Array that stores keys for all procedure entries.
        wali::Key * entries;
        // Array that stores keys for all procedure exits.
        wali::Key * exits;
        // Name of the pds state to use
        wali::Key pdsState;
    };
  } // namespace wpds
} // namespace wali
#endif //wali_genereate_Random_FWPDS_guard