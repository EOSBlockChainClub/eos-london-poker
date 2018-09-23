#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
using namespace eosio;

// Smart Contract Name: notechain
// Table struct:
//   notestruct: multi index table to store the notes
//     prim_key(uint64): primary key
//     user(account_name/uint64): account name for the user
//     note(string): the note message
//     timestamp(uint64): the store the last update block time
// Public method:
//   isnewuser => to check if the given account name has note in table or not
// Public actions:
//   update => put the note into the multi-index table and sign by the given account

// Replace the contract class name when you start your own project
class notechain : public eosio::contract
{
  private:

  public:
    using contract::contract;

    /// @abi action
    void update( account_name _user, std::string& _note )
	{
		
    }

};

// specify the contract name, and export a public action: update
EOSIO_ABI( notechain, (update) )
