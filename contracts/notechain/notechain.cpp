#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/system.h>
#include <eosio.token/eosio.token.hpp>

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
	
	// we need this struct and table to access eosio.token balances
	struct account
	{
		asset balance;

		uint64_t primary_key()const { return balance.symbol.name(); }
	};
    typedef eosio::multi_index<N(accounts), account> accounts;

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

		auto table_it = datas.find(table_id);
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

		auto table_it = datas.find(table_id);
		assert(table_it != datas.end());
		assert(table_it->state == TABLE_READY);
		assert((_self == table_it->alice) || (_self == table_it->bob));

		// check if other player is ready
		bool ready = (_self == table_it->alice) ? table_it->bob_ready : table_it->alice_ready;
		if (!ready)
		{
			// other player is not ready, wait for them
			datas.modify(table_it, _self, [&](auto& table) {
				if (_self == table.alice)
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
				table.alice_keys = vector<checksum256>(53);
				table.bob_keys = vector<checksum256>(53);
			});
		}
		// now we should hold the bankroll amount of money on user account (hard-coded for now)
		// FIXME: this stake is lost if game is cancelled, but game cancellation is not handled under hackathon time pressure
		asset newBalance(table_it->buy_in.amount * 2, table_it->buy_in.symbol);
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
		/* encrypts card with commutative cryptography algorithm */
		/* XOR is not secure and is only used for illustration purposes. ElGamal/SRA are okay with certain params. */
		
		// xor card with pk
		return card;
	}
	// SIMPLIFIED FOR HACKATHON (only needed to finalize on-chain cheating detection, trivial to implement)
	checksum256 decrypt(checksum256 card, checksum256 pk)
	{
		/* decrypts card with commutative cryptography algorithm */
		/* XOR is not secure and is only used for illustration purposes. ElGamal/SRA are okay with certain params. */

		// xor card with pk
		return card;
	}

	///////////////////////// SHUFFLING METHODS ////////////////////////////

	/// @abi action
	void deck_shuffled(uint64_t table_id, vector<checksum256> encrypted_cards)
	{
		/* player pushes shuffled & encrypted deck */

		rounddatas datas(_self, _self);

		auto table_it = datas.find(table_id);
		assert(table_it != datas.end());
		assert(table_it->state == SHUFFLE);
		assert(_self == table_it->target);

		if (_self == table_it->alice)
		{
			// first player has shuffled the cards, we need to give cards to another player
			datas.modify(table_it, _self, [&](auto& table) {
				// table.state = SHUFFLE; // state doesn't change
				table.target = table.bob;
				table.encrypted_cards = encrypted_cards;
			});
		}
		else
		{
			// both players have shuffled the cards, we can proceed with re-encryprion
			datas.modify(table_it, _self, [&](auto& table) {
				table.state = RECRYPT;
				table.target = table.alice;
				table.encrypted_cards = encrypted_cards;
			});
		}
	}
	/// @abi action
	void deck_recrypted(uint64_t table_id, vector<checksum256> encrypted_cards)
	{
		/* player pushes re-encrypted deck */

		rounddatas datas(_self, _self);

		auto table_it = datas.find(table_id);
		assert(table_it != datas.end());
		assert(table_it->state == RECRYPT);
		assert(_self == table_it->target);

		if (_self == table_it->alice)
		{
			// first player has re-encrypted the cards, we need to give cards to another player
			datas.modify(table_it, _self, [&](auto& table) {
				// table.state = SHUFFLE; // state doesn't change
				table.target = table.bob;
				table.encrypted_cards = encrypted_cards;
			});
		}
		else
		{
			// both players have re-encrypted the cards, we can proceed to game
			datas.modify(table_it, _self, [&](auto& table) {
				table.state = DEAL_POCKET;
				table.target = table.alice;
				table.cards_dealt = 0;
				table.encrypted_cards = encrypted_cards;
			});
		}
	}
	/// @abi action
	void card_key(uint64_t table_id, checksum256 key)
	{
		/* receive next card private key from player */
		
		rounddatas datas(_self, _self);

		auto table_it = datas.find(table_id);
		assert(table_it != datas.end());
		assert((table_it->state == DEAL_TABLE) || (table_it->state == DEAL_POCKET));
		assert((_self == table_it->alice) || (_self == table_it->bob));
		if (table_it->state == DEAL_POCKET)
		{
			// we're dealing pocket cards
			
			assert(_self != table_it->target); // we should not send encryption keys for our own cards

			if (_self == table_it->alice)
			{
				datas.modify(table_it, _self, [&](auto& table) {
					// save the key for later use in decryption
					table.alice_keys[table.cards_dealt + 1] = key;

					table.cards_dealt = table.cards_dealt + 1;

					table.target = table.bob;
				});
			}
			else
			{
				datas.modify(table_it, _self, [&](auto& table) {
					// save the key for later use in decryption
					table.bob_keys[table.cards_dealt + 1] = key;

					table.cards_dealt = table.cards_dealt + 1;

					table.target = table.alice;

					if (table.cards_dealt == 4) // hardcoded pocket cards count (2 players with 2 pocket cards each)
					{
						// we dealt 2 cards to each player, starting betting round
						table.state = BET_ROUND;
					}
				});
			}
		}
		else
		{
			// we're dealing table cards
			auto opponent_keys = (_self == table_it->alice) ? table_it->bob_keys : table_it->alice_keys;
			// check if the opponent has already sent their keys
			if (opponent_keys[table_it->cards_dealt + 1] == checksum256())
			{
				// opponent is not ready yet
				datas.modify(table_it, _self, [&](auto& table) {
					// save the card key for later use
					if (_self == table.alice)
					{
						table.alice_keys[table_it->cards_dealt + 1] = key;
					}
					else
					{
						table.bob_keys[table_it->cards_dealt + 1] = key;
					}
				});
			}
			else
			{
				// opponent has already given their key
				datas.modify(table_it, _self, [&](auto& table) {
					// save the card key for later use
					if (_self == table.alice)
					{
						table.alice_keys[table_it->cards_dealt + 1] = key;
					}
					else
					{
						table.bob_keys[table_it->cards_dealt + 1] = key;
					}
					
					// one more card is marked as dealt
					table.cards_dealt = table.cards_dealt + 1;

					if (table.cards_dealt < 7) // magic number 7 is `2(alice cards) + 2(bob cards) + 3 (flop cards)`
					{
						// flop was not fully dealt yet, waiting for more keys
						return;
					}
					else
					{
						// it's either flop, turn, or river
						// we don't burn card like they do in casinos, it has no effect on randomness
						// but we can burn it if we decide to
						table.state = BET_ROUND;
						table.target = table.target; // there's a little mess with turn order, but we have no more hackathon time to fix it
					}
				});
			}
		}
	}
	
	//////////////////////// POKER GAME LOGIC METHODS ////////////////////////////
	
	/// @abi action
	void check(uint64_t table_id)
	{
		/* `check` (do not raise bet, do not fold cards) */

		rounddatas datas(_self, _self);

		auto table_it = datas.find(table_id);
		assert(table_it != datas.end());
		assert(table_it->state == BET_ROUND);
		assert(_self == table_it->target);

		// we can check only if bets are equal
		assert(table_it->alice_bet == table_it->bob_bet);

		if (_self == table_it->alice)
		{
			// first action, let the other one decide
			datas.modify(table_it, _self, [&](auto& table) {
				table.target = table.bob;
			});
		}
		else
		{
			datas.modify(table_it, _self, [&](auto& table) {
				if (table.cards_dealt < 9) // magic number 9 is `2(alice cards) + 2(bob cards) + 3 (flop cards) + 1 (turn card) + 1 (river card)`
				{
					table.state = DEAL_TABLE;
	}
				else
				{
					// calculate winner!
				}
			});
		}
	}
	/// @abi action
	void call(uint64_t table_id)
	{
		/* `call` (raise bet to match opponent raised bet) */

		rounddatas datas(_self, _self);

		auto table_it = datas.find(table_id);
		assert(table_it != datas.end());
		assert(table_it->state == BET_ROUND);
		assert(_self == table_it->target);
	}
	/// @abi action
	void raise(uint64_t table_id, eosio::asset amount)
	{
		/* `raise` bet (no more than current player bankroll) */

		rounddatas datas(_self, _self);

		auto table_it = datas.find(table_id);
		assert(table_it != datas.end());
		assert(table_it->state == BET_ROUND);
		assert(_self == table_it->target);
	}
	/// @abi action
	void fold(uint64_t table_id)
	{
		/* `fold` (drop cards, stop playing current round) */
		
		rounddatas datas(_self, _self);

		auto table_it = datas.find(table_id);
		assert(table_it != datas.end());
		assert(table_it->state == BET_ROUND);
		assert(_self == table_it->target);
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
