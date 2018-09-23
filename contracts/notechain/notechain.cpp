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

		// current state of the game
		roundstatename state;
		// target player (behavior depends on current state)
		account_name target;

		// first player
		account_name alice;
		// second player
		account_name bob;

		// bankroll (total available money)
		eosio::asset alice_bankroll;
		eosio::asset bob_bankroll;

		// current round bets
		eosio::asset alice_bet;
		eosio::asset bob_bet;

		// amount of money needed to enter this table
		eosio::asset buy_in;

		// pocket cards
		checksum256 alice_card_1;
		checksum256 alice_card_2;
		checksum256 bob_card_1;
		checksum256 bob_card_2;

		// table cards
		vector<checksum256> table_cards;

		// amount of cards that came into play
		uint8_t cards_dealt;

		// array of encrypted cards in a deck
		vector<checksum256> encrypted_cards;

		// player private keys (PK_0, PK_1-PK_52)
		vector<checksum256> alice_keys;
		vector<checksum256> bob_keys;

		// whether the players are ready to play
		bool alice_ready;
		bool bob_ready;

		auto primary_key() const { return table_id; }
	};

	typedef eosio::multi_index< N(rounddata), rounddata
    //   indexed_by< N(getbyuser), const_mem_fun<notestruct, account_name, &notestruct::get_by_user> >
      > rounddatas;

	//////////// GAME SEARCH SIMPLIFIED FOR HACKATHON ////////////

	/// @abi action
	void search_game( /* asset min_stake, asset max_stake */ ) // only one bet possible for hackathon
	{
		/* player searches a suitable table (for hackathon any table is suitable) */

		rounddatas datas(_self, _self);

		for (auto table_it = datas.begin(); table_it != datas.end(); ++table_it)
		{
			if (table_it->state != WAITING_FOR_PLAYERS)
			{
				// skip already playing tables
				continue;
			}
			if (table_it->alice == _self)
			{
				// can't play with myself
				continue;
			}
			if (table_it->bob == account_name())
			{
				// found suitable table, let's join it

				datas.modify(table_it, _self, [&]( auto& table ) {
					table.bob = _self;
					table.state = TABLE_READY;
				});

				return;
			}
		}
		
		// couldn't find suitable table, let's create a new one
		
		datas.emplace(_self, [&]( auto& table ) {
			table.table_id = datas.available_primary_key();
			table.alice = _self;
			table.state = WAITING_FOR_PLAYERS;
			// table buy-in is hardcoded for the duration of hackathon
			table.buy_in = asset(1000, CORE_SYMBOL);
        });
	}

	/// @abi action
	void cancel_game(uint64_t table_id)
	{
		/* cancel game before the start */
		rounddatas datas(_self, _self);

		auto table_it = datas.get(table_id);
		assert(table_it != datas.end());
		assert((table_it->state == WAITING_FOR_PLAYERS) || (table_it->state == TABLE_READY));
		assert((_self == table_it->alice) || (_self == table_it->bob));
		if (_self == table_it->alice)
		{
			// this is the user that created table (alice)
			datas.modify(table_it, _self, [&](auto& table) {
				// make other player (bob) the creator
				table.alice = table.bob;
				table.bob = account_name();
				table.state = WAITING_FOR_PLAYERS;
				table.alice_ready = false;
				table.bob_ready = false;
			});
		}
		else
	{
			// just remove the player from table
			datas.modify(table_it, _self, [&](auto& table) {
				table.bob = account_name();
				table.state = WAITING_FOR_PLAYERS;
				table.alice_ready = false;
				table.bob_ready = false;
			});
		}
	}

    /// @abi action
    void start_game(uint64_t table_id)
	{
		rounddatas datas(_self, _self);

		auto table_it = datas.get(table_id);
		assert(table_it != datas.end());
		assert(table_it->state == TABLE_READY);
		assert((_self == table_it->alice) || (_self == table_it->bob));

		// check if other player is ready
		bool ready = (_self == table_it->alice) ? table_it->bob_ready : table_it->alice_ready;
		if (!ready)
		{
			// other player is not ready, wait for them
			datas.modify(table_it, _self, [&](auto& table) {
				if (_self == table->alice)
				{
					table.alice_ready = true;
				}
				else
				{
					table.bob_ready = true;
				}
			});
		}
		else
		{
			// both players are ready, we can start the game and shuffle cards
			datas.modify(table_it, _self, [&](auto& table) {
				table.state = SHUFFLE;
				table.target = table.alice;
				table.cards_dealt = 0;
				table.encrypted_cards = vector(53);
				table.alice_keys = vector(53);
				table.bob_keys = vector(53);
			});
		}
		// now we should hold the bankroll amount of money on user account (hard-coded for now)
		// FIXME: this stake is lost if game is cancelled, but game cancellation is not handled under hackathon time pressure
		asset newBalance(table_it->buy_in * 2, CORE_SYMBOL);
		action(
			permission_level{ _self, N(active) },
			N(eosio.token), N(transfer),
			std::make_tuple(_self, N(notechainacc), newBalance, std::to_string(table_it->table_id))
        ).send();
    }
	bool enoughMoney(account_name opener, asset quantity)
	{
		return getUserBalance(opener, quantity).amount >= quantity.amount;
	}
	asset getUserBalance(account_name opener, asset quantity)
	{
		accounts acc( N(eosio.token), opener );
		auto balance = acc.get(quantity.symbol.name());
		return balance.balance;
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
	void deck_shuffled(uint64_t table_id, vector<checksum256> encrypted_cards)
	{
		/* player pushes shuffled & encrypted deck */

	}
	/// @abi action
	void deck_recrypted(uint64_t table_id, vector<checksum256> encrypted_cards)
	{
		/* player pushes re-encrypted deck */

	}
	/// @abi action
	void card_key(uint64_t table_id, checksum256 key)
	{
		/* receive next card private key from player */

	}
	
	//////////////////////// POKER GAME LOGIC METHODS ////////////////////////////
	
	/// @abi action
	void check(uint64_t table_id)
	{
		/* `check` (do not raise bet, do not fold cards) */

	}
	/// @abi action
	void call(uint64_t table_id)
	{
		/* `call` (raise bet to match opponent raised bet) */

	}
	/// @abi action
	void raise(uint64_t table_id, eosio::asset amount)
	{
		/* `raise` bet (no more than current player bankroll) */

	}
	/// @abi action
	void fold(uint64_t table_id)
	{
		/* `fold` (drop cards, stop playing current round) */
	}

	///////////////////// DISPUTES & CHEATING DETECTION ////////////////////

	/// @abi action
	void dispute(uint64_t table_id)
	{
		/* Open cheating dispute. The disputing player has to stake total value of all bankrolls on the table. */

	}
	/// @abi action
	void card_keys(uint64_t table_id, vector<checksum256> private_keys)
	{
		/* Receives all card encryption private keys from the player to check for cheating. */

	}
	/// @abi action
	void dispute_step(uint64_t table_id, uint8_t step_idx)
	{
		/* Dispute a specific encryption step. For optimization only
			(players can do their calculation off-chain and then check just one). */
		
	}
};

EOSIO_ABI( poker, (search_game)(cancel_game)(start_game)(deck_shuffled)(deck_recrypted)(card_key)(check)(call)(raise)(fold)(dispute)(card_keys)(dispute_step) )
