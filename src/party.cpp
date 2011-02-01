
/* $Id$
 * EOSERV is released under the zlib license.
 * See LICENSE.txt for more info.
 */

#include "party.hpp"

#include <cmath>

#include "container/algorithm.hpp"
#include "util.hpp"

#include "character.hpp"
#include "eoclient.hpp"
#include "packet.hpp"
#include "player.hpp"
#include "world.hpp"

Party::Party(World *world, Character *leader, Character *other)
{
	this->world = world;
	this->world->parties.push_back(this);

	this->members.push_back(leader);
	this->members.push_back(other);

	this->leader = leader;

	leader->party = this;
	other->party = this;

	this->temp_expsum = 0;

	this->RefreshMembers(leader);
	this->RefreshMembers(other);
}

void Party::Msg(Character *from, std::string message, bool echo)
{
	message = util::text_cap(message, static_cast<int>(this->world->config["ChatMaxWidth"]) - util::text_width(util::ucfirst(from->name) + "  "));

	PacketBuilder builder(PACKET_TALK, PACKET_OPEN);

	builder.AddShort(from->player->id);
	builder.AddString(message);

	UTIL_PTR_VECTOR_FOREACH(this->members, Character, member)
	{
		if (echo || *member != from)
		{
			member->player->client->SendBuilder(builder);
		}
	}
}

void Party::Join(Character *character)
{
	character->party = this;

	this->members.push_back(character);

	PacketBuilder builder(PACKET_PARTY, PACKET_ADD);
	builder.AddShort(character->player->id);
	builder.AddChar(character == this->leader);
	builder.AddChar(character->level);
	builder.AddChar(int(double(character->hp) / double(character->maxhp) * 100.0));
	builder.AddString(character->name);

	UTIL_PTR_VECTOR_FOREACH(this->members, Character, checkcharacter)
	{
		if (*checkcharacter != character)
		{
			checkcharacter->player->client->SendBuilder(builder);
		}
	}

	this->RefreshMembers(character);
}

void Party::Leave(Character *character)
{
	if (this->members.size() > 2 && character != this->leader)
	{
		UTIL_PTR_VECTOR_FOREACH(this->members, Character, checkcharacter)
		{
			if (*checkcharacter == character)
			{
				this->members.erase(checkcharacter);
				break;
			}
		}

		character->party = 0;

		PacketBuilder builder(PACKET_PARTY, PACKET_REMOVE);
		builder.AddShort(character->player->id);
		UTIL_PTR_VECTOR_FOREACH(this->members, Character, checkcharacter)
		{
			if (character != *checkcharacter)
			{
				checkcharacter->player->client->SendBuilder(builder);
			}
		}

		builder.Reset();
		builder.SetID(PACKET_PARTY, PACKET_CLOSE);
		builder.AddShort(255);
		character->player->client->SendBuilder(builder);
	}
	else
	{
		this->Destroy();
	}
}

void Party::RefreshMembers(Character *character)
{
	PacketBuilder builder(PACKET_PARTY, PACKET_CREATE);

	UTIL_PTR_VECTOR_FOREACH(this->members, Character, member)
	{
		builder.AddShort(member->player->id);
		builder.AddChar(*member == this->leader);
		builder.AddChar(member->level);
		builder.AddChar(int(double(member->hp) / double(member->maxhp) * 100.0));
		builder.AddBreakString(member->name);
	}

	character->player->client->SendBuilder(builder);
}

void Party::UpdateHP(Character *character)
{
	PacketBuilder builder(PACKET_PARTY, PACKET_AGREE);
	builder.AddShort(character->player->id);
	builder.AddChar(int(double(character->hp) / double(character->maxhp) * 100.0));

	UTIL_PTR_VECTOR_FOREACH(this->members, Character, member)
	{
		member->player->client->SendBuilder(builder);
	}
}

void Party::ShareEXP(int exp, int sharemode, Map *map)
{
	int reward = 0;
	double sumlevel = 0;
	double members = 0;

	UTIL_PTR_VECTOR_FOREACH(this->members, Character, member)
	{
		if (member->map == map && !member->nowhere)
		{
			if (member->level == 0)
			{
				++sumlevel;
			}
			else
			{
				sumlevel += member->level;
			}
			++members;
		}
	}

	UTIL_PTR_VECTOR_FOREACH(this->members, Character, member)
	{
		if (member->map != map || member->nowhere)
		{
			continue;
		}
		switch (sharemode)
		{
			case 1:
				reward = int(std::ceil(double(exp) / members));
				break;

			case 2:
				reward = int(std::ceil(double(exp) * double((member->level == 0) ? 1 : member->level) / sumlevel));
				break;
		}

		// TODO: Levelling up in this way doesn't work well, find alternative.

		member->exp += reward;

		bool level_up = (member->level < static_cast<int>(this->world->config["MaxLevel"]) && member->exp >= this->world->exp_table[member->level+1]);

		PacketBuilder builder(PACKET_PARTY, PACKET_EXP);
		builder.AddShort(member->player->id);
		builder.AddInt(reward);
		builder.AddChar(level_up);

		if (level_up)
		{
			++member->level;
			member->statpoints += static_cast<int>(this->world->config["StatPerLevel"]);
			member->skillpoints += static_cast<int>(this->world->config["SkillPerLevel"]);
			member->CalculateStats();
		}

		member->player->client->SendBuilder(builder);
	}
}

Party::~Party()
{
	if (this->world)
	{
		erase_first(this->world->parties, this);
	}

	PacketBuilder builder(PACKET_PARTY, PACKET_CLOSE);
	builder.AddByte(255); // ?

	UTIL_PTR_VECTOR_FOREACH(this->members, Character, member)
	{
		member->party = 0;
		member->player->client->SendBuilder(builder);
	}
}
