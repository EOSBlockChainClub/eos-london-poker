#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/system.h>

using namespace eosio;

struct playerpair
{
	uint64_t alice;
	uint64_t bob;
};

class poker : public eosio::contract
{
  private:

  public:
    using contract::contract;

	enum roundstatename
	{
		NOT_STARTED,
		WAITING_FOR_PLAYERS,
		TABLE_READY,
		SHUFFLE,
		RECRYPT,
		DEAL_POCKET,
		BET_ROUND,
		DEAL_TABLE,
		END
	};
	/// @abi table rounddatas
	struct rounddata
	{
		uint64_t table_id;

		roundstatename state;
		account_name target;

		account_name alice;
		account_name bob;

		asset alice_bankroll;
		asset bob_bankroll;
		asset alice_bet;
		asset bob_bet;

		checksum256 alice_card_1;
		checksum256 alice_card_2;
		checksum256 bob_card_1;
		checksum256 bob_card_2;

		uint8_t cards_dealt;
		vector<checksum256> encrypted_cards;

		vector<checksum256> alice_keys;
		vector<checksum256> bob_keys;

		bool alice_ready;
		bool bob_ready;
	};

	//////////// GAME SEARCH SIMPLIFIED FOR HACKATHON ////////////

	/// @abi action
	void search_game( /* asset min_stake, asset max_stake */ ) // only one bet possible for hackathon
	{

	}

	/// @abi action
	void cancel_game()
	{
		_self;
	}

    /// @abi action
    void start_game()
	{
		_self;
    }

	///////////////////////////////////////////////////////////

	// SIMPLIFIED FOR HACKATHON (only needed to finalize on-chain cheating detection, trivial to implement)
	checksum256 encrypt(checksum256 card, checksum256 pk)
	{
		/* encrypts card with commutative algorithm */
		/* XOR is not secure and is only used for illustration purposes. ElGamal/SRA are okay with certain params. */
		
		// xor card with pk
	}
	// SIMPLIFIED FOR HACKATHON (only needed to finalize on-chain cheating detection, trivial to implement)
	checksum256 decrypt(checksum256 card, checksum256 pk)
	{
		/* decrypts card with commutative algorithm */
		/* XOR is not secure and is only used for illustration purposes. ElGamal/SRA are okay with certain params. */

		// xor card with pk
	}

	///////////////////////// SHUFFLING METHODS ////////////////////////////

	/// @abi action
	void deck_shuffled(vector<checksum256> encrypted_cards)
	{
		/* player pushes shuffled & encrypted deck */

	}
	/// @abi action
	void deck_recrypted(vector<checksum256> encrypted_cards)
	{
		/* player pushes re-encrypted deck */

	}
	/// @abi action
	void card_key(checksum256 key)
	{
		/* receive next card private key from player */

	}
	
	//////////////////////// POKER GAME LOGIC METHODS ////////////////////////////
	
	/// @abi action
	void check()
	{
		/* `check` (do not raise bet, do not fold cards) */

	}
	/// @abi action
	void call()
	{
		/* `call` (raise bet to match opponent raised bet) */

	}
	/// @abi action
	void raise(asset amount)
	{
		/* `raise` bet (no more than current player bankroll) */

	}
	/// @abi action
	void fold()
	{
		/* `fold` (drop cards, stop playing current round) */
	}

	///////////////////// DISPUTES & CHEATING DETECTION ////////////////////

	/// @abi action
	void dispute()
	{
		/* Open cheating dispute. The disputing player has to stake total value of all bankrolls on the table. */

	}
	/// @abi action
	void card_keys(vector<checksum256> private_keys)
	{
		/* Receives all card encryption private keys from the player to check for cheating. */

	}
	/// @abi action
	void dispute_step(uint8_t step_idx)
	{
		/* Dispute a specific encryption step. For optimization only
			(players can do their calculation off-chain and then check just one). */
		
	}
};

EOSIO_ABI( poker, (search_game)(cancel_game)(start_game)(deck_shuffled)(deck_recrypted)(card_key)(check)(call)(raise)(fold)(dispute)(card_keys)(dispute_step) )
