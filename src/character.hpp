
/* $Id$
 * EOSERV is released under the zlib license.
 * See LICENSE.txt for more info.
 */

#ifndef CHARACTER_HPP_INCLUDED
#define CHARACTER_HPP_INCLUDED

#include "fwd/character.hpp"

#include <array>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <string>

#include "fwd/arena.hpp"
#include "fwd/guild.hpp"
#include "fwd/npc.hpp"
#include "fwd/map.hpp"
#include "fwd/packet.hpp"
#include "fwd/party.hpp"
#include "fwd/player.hpp"
#include "fwd/quest.hpp"
#include "fwd/world.hpp"

#include "command_source.hpp"
#include "eodata.hpp"
#include "guild.hpp"

struct Timestamp
{
	private:
		int ts;
		static const int max_ts = 8640000;

	public:
		Timestamp()
			: ts(-1)
		{ }

		Timestamp(int ts)
			: ts(ts % max_ts)
		{ }

		int GetHour() const        { return ts / 360000; }
		int GetMinute() const      { return ts / 6000 - GetHour() * 60; }
		int GetSecond() const      { return ts / 100 - GetHour() * 3600 - GetMinute() * 60; }
		int GetMillisecond() const { return ts * 10 - GetHour() * 3600000 - GetMinute() * 60000 - GetSecond() * 1000; }

		int operator-(Timestamp b) const
		{
			if (ts == -1) return b.ts;
			if (b.ts == -1) return ts;

			if (b.ts > ts)
				return ts - b.ts + max_ts;
			else
				return ts - b.ts;
		}
};

void character_cast_spell(void *character_void);

/**
 * Serialize a list of items in to a text format that can be restored with ItemUnserialize
 */
std::string ItemSerialize(const std::list<Character_Item> &list);

/**
 * Convert a string generated by ItemSerialze back to a list of items
 */
std::list<Character_Item> ItemUnserialize(const std::string& serialized);

/**
 * Serialize a paperdoll of 15 items in to a string that can be restored with DollUnserialize
 */
std::string DollSerialize(const std::array<int, 15> &list);

/**
 * Convert a string generated by DollSerialze back to a list of 15 items
 */
std::array<int, 15> DollUnserialize(const std::string& serialized);

/**
 * Serialize a list of spells in to a text format that can be restored with SpellUnserialize
 */
std::string SpellSerialize(const std::list<Character_Spell> &list);

/**
 * Convert a string generated by SpellSerialze back to a list of items
 */
std::list<Character_Spell> SpellUnserialize(const std::string& serialized);

/**
 * One type of item in a Characters inventory
 */
struct Character_Item
{
	short id;
	int amount;

	Character_Item() = default;
	Character_Item(short id, int amount) : id(id), amount(amount) { }
};

/**
 * One spell that a Character knows
 */
struct Character_Spell
{
	short id;
	unsigned char level;

	Character_Spell() = default;
	Character_Spell(short id, unsigned char level) : id(id), level(level) { }
};

struct Character_QuestState
{
	short quest_id;
	std::string quest_state;
	std::string quest_progress;

	bool operator <(const Character_QuestState& rhs) const
	{
		return this->quest_id < rhs.quest_id;
	}
};

class Character : public Command_Source
{
	public:
		enum HideFlag
		{
			HideInvisible = 0x01,
			HideOnline    = 0x02,
			HideNpc       = 0x04,
			HideAdmin     = 0x08,
			HideWarp      = 0x10,

			HideOnlineExplicit    = 0x020000,
			HideNpcExplicit       = 0x040000,
			HideAdminExplicit     = 0x080000,
			HideWarpExplicit      = 0x100000
		};

		static constexpr int HideAll = 0x1F;

		int login_time;
		bool online;
		bool nowhere;
		unsigned int id;
		AdminLevel admin;
		std::string name;
		std::string title;
		std::string home;
		std::string fiance;
		std::string partner;
		unsigned char clas;
		Gender gender;
		Skin race;
		unsigned char hairstyle, haircolor;
		short mapid;
		unsigned char x, y;
		Direction direction;
		unsigned char level;
		int exp;
		short hp, tp;
		short str, intl, wis, agi, con, cha;
		short adj_str, adj_intl, adj_wis, adj_agi, adj_con, adj_cha;
		short statpoints, skillpoints;
		short weight, maxweight;
		short karma;
		SitState sitting;
		int hidden;
		bool whispers;
		int bankmax;
		int goldbank;
		int usage;
		int muted_until;
		bool bot;

		Arena *next_arena;
		Arena *arena;
		char arena_kills;

		short maxsp;
		short maxhp, maxtp;
		short accuracy, evade, armor;
		short mindam, maxdam;

		bool trading;
		Character *trade_partner;
		bool trade_agree;
		std::list<Character_Item> trade_inventory;

		Character *party_trust_send;
		Character *party_trust_recv;
		PartyRequestType party_send_type;

		NPC *npc;
		ENF::Type npc_type;
		Board *board;
		bool jukebox_open;
		std::string guild_join;
		std::string guild_invite;

		Timestamp timestamp;

		std::deque<std::string> chat_log;

		enum SpellTarget
		{
			TargetInvalid,
			TargetSelf,
			TargetNPC,
			TargetPlayer,
			TargetGroup
		};

		bool spell_ready;
		unsigned short spell_id;
		TimeEvent *spell_event;
		SpellTarget spell_target;
		unsigned short spell_target_id;

		double last_walk;
		int attacks;

		WarpAnimation warp_anim;

		enum EquipLocation
		{
			Boots,
			Accessory,
			Gloves,
			Belt,
			Armor,
			Necklace,
			Hat,
			Shield,
			Weapon,
			Ring1,
			Ring2,
			Armlet1,
			Armlet2,
			Bracer1,
			Bracer2
		};

		std::list<Character_Item> inventory;
		std::list<Character_Item> bank;
		std::array<int, 15> paperdoll;
		std::array<int, 15> cosmetic_paperdoll;
		std::list<Character_Spell> spells;
		std::list<NPC *> unregister_npc;
		std::map<short, std::shared_ptr<Quest_Context>> quests;
		std::set<Character_QuestState> quests_inactive;
		std::string quest_string;

		Character(std::string name, World *);

		bool IsHideInvisible() const { return hidden & HideInvisible; }
		bool IsHideOnline() const { return hidden & HideOnline; }
		bool IsHideNpc() const { return hidden & HideNpc; }
		bool IsHideAdmin() const { return hidden & HideAdmin; }
		bool IsHideWarp() const { return hidden & HideWarp; }

		void Login();

		static bool ValidName(std::string name);

		void Msg(Character *from, std::string message);
		void ServerMsg(std::string message);
		void StatusMsg(std::string message);
		bool Walk(Direction direction);
		bool AdminWalk(Direction direction);
		void Attack(Direction direction);
		void Sit(SitState sit_type);
		void Stand();
		void Emote(enum Emote emote, bool echo = true);
		void Effect(int effect, bool echo = true);
		void PlayBard(unsigned char instrument, unsigned char note, bool echo = true);
		int HasItem(short item, bool include_trade = false);
		bool HasSpell(short spell);
		short SpellLevel(short spell);
		bool AddItem(short item, int amount);
		bool DelItem(short item, int amount);
		int CanHoldItem(short item, int max_amount);
		std::list<Character_Item>::iterator DelItem(std::list<Character_Item>::iterator, int amount);
		bool AddTradeItem(short item, int amount);
		bool DelTradeItem(short item);
		bool AddSpell(short spell);
		bool DelSpell(short spell);
		void CancelSpell();
		void SpellAct();
		bool Unequip(short item, unsigned char subloc);
		bool Equip(short item, unsigned char subloc);
		bool InRange(unsigned char x, unsigned char y) const;
		bool InRange(const Character *) const;
		bool InRange(const NPC *) const;
		bool InRange(const Map_Item&) const;
		void Warp(short map, unsigned char x, unsigned char y, WarpAnimation animation = WARP_ANIMATION_NONE);
		void Refresh();
		void ShowBoard(Board *board = 0);
		std::string PaddedGuildTag();
		int Usage();
		short SpawnMap();
		unsigned char SpawnX();
		unsigned char SpawnY();
		void CheckQuestRules();
		void CalculateStats(bool trigger_quests = true);
		void DropAll(Character *killer);
		void Hide(int setflags);
		void Unhide(int unsetflags);
		void Reset();
		std::shared_ptr<Quest_Context> GetQuest(short id);
		void ResetQuest(short id);

		void Mute(const Command_Source *by);
		void PlaySound(unsigned char id);

		void FormulaVars(std::unordered_map<std::string, double> &vars, std::string prefix = "");

		void Dress(EquipLocation, unsigned short gfx_id);
		void Undress();
		void Undress(EquipLocation);
		void AddPaperdollData(PacketBuilder&, const char* format);

		void AddChatLog(std::string marker, std::string name, std::string msg);
		std::string GetChatLogDump();

		void Send(const PacketBuilder &);

		void Logout();
		void Save();

		AdminLevel SourceAccess() const;
		std::string SourceName() const;
		Character* SourceCharacter();
		World* SourceWorld();

		~Character();

		World *world;
		Player *player;
		std::shared_ptr<Guild> guild;
		std::shared_ptr<Guild_Create> guild_create;
		unsigned char guild_rank;
		Party *party;
		Map *map;

		const short &display_str, &display_intl, &display_wis, &display_agi, &display_con, &display_cha;
};

#endif // CHARACTER_HPP_INCLUDED
