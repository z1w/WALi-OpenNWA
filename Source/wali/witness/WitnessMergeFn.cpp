/**
 * @author Nicholas Kidd
 *
 * @version $Id$
 */

#include "wali/witness/WitnessMerge.hpp"
#include "wali/witness/WitnessMergeFn.hpp"

namespace wali 
{
  namespace witness 
  {
    WitnessMergeFn::WitnessMergeFn( witness_t witness_rule, merge_fn_t user_merge )
      : MergeFn(),witness_rule(witness_rule),user_merge(user_merge) 
    {
    }

    WitnessMergeFn::~WitnessMergeFn() 
    {
    }

    sem_elem_t WitnessMergeFn::apply_f( sem_elem_t a, sem_elem_t b)
    {
      return priv_do_apply(a,b);
    }

    std::ostream& WitnessMergeFn::print( std::ostream& o ) const
    {
      o << "WitnessMergeFn[ ";
      user_merge->print(o) << "]";
      return o;
    }

    sem_elem_t WitnessMergeFn::priv_do_apply( sem_elem_t a,sem_elem_t b )
    {
      Witness* left = dynamic_cast< Witness* >(a.get_ptr());
      Witness* right = dynamic_cast< Witness* >(b.get_ptr());
      if( left == 0 ) {
        *waliErr << "[ERROR] Attempt to apply WitnessMergeFn to non witness.\n";
        assert(0);
      }
      if( right == 0 ) {
        *waliErr << "[ERROR] Attempt to apply WitnessMergeFn to non witness.\n";
        assert(0);
      }

      sem_elem_t user_se = user_merge->apply_f( left->weight(), right->weight());

      WitnessMerge* witmerge = new WitnessMerge(user_se,this,left,witness_rule,right);
      return witmerge;
    }

    merge_fn_t WitnessMergeFn::get_user_merge()
    {
      return user_merge;
    }

  } // namespace witness

} // namespace wali

