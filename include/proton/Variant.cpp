#include "pch.hpp"

#include "Variant.hpp"

void Variant::Set( std::string const &var )
{
	m_type = TYPE_STRING;
	m_string = var;
}

int GetSizeOfData(Variant::eType type)
{
	switch(type)
	{
	case Variant::TYPE_UNUSED:
	case Variant::TYPE_COMPONENT:
	case Variant::TYPE_ENTITY:
		return 0;
		break;

	case Variant::TYPE_UINT32:
	case Variant::TYPE_INT32:
	case Variant::TYPE_FLOAT:
		return 4;
	case Variant::TYPE_VECTOR2:
		return sizeof(CL_Vec2f);
	case Variant::TYPE_VECTOR3:
		return sizeof(CL_Vec3f);

	}

	return 0;
}

u_char * VariantList::SerializeToMem( u_int *pSizeOut, u_char *pDest )
{
	int varsUsed = 0;
	int memNeeded = 0;

	int tempSize;

	for (int i=0; i < C_MAX_VARIANT_LIST_PARMS; i++)
	{
		if (m_variant[i].GetType() == Variant::TYPE_STRING)
		{
			tempSize = (int)m_variant[i].GetString().size()+4; //the 4 is for an int showing how long the string will be when writing
		} else
		{
			tempSize = GetSizeOfData(m_variant[i].GetType());
		}

		if (tempSize > 0)
		{
			varsUsed++;
			memNeeded += tempSize;
		}
	}

	int totalSize = memNeeded + 1 + (varsUsed*2);


	if (!pDest)
	{
		pDest = new u_char[totalSize]; //1 is to write how many are coming
	}

	//write it

	
	u_char *pCur = pDest;

	pCur[0] = u_char(varsUsed);
	pCur++;

	u_char type;

	for (int i=0; i < C_MAX_VARIANT_LIST_PARMS; i++)
	{
		if (m_variant[i].GetType() == Variant::TYPE_STRING)
		{
			type = i;
			memcpy(pCur, &type, 1); pCur += 1; //index

			type = Variant::TYPE_STRING;
			memcpy(pCur, &type, 1); pCur += 1; //type

			
			u_int s = (int)m_variant[i].GetString().size();
			memcpy(pCur, &s, 4); pCur += 4; //length of string
			memcpy(pCur, m_variant[i].GetString().c_str(), s); pCur += s; //actual string data
		} else
		{
			tempSize = GetSizeOfData(m_variant[i].GetType());

			if (tempSize > 0)
			{
				type = i;
				memcpy(pCur, &type, 1); pCur += 1; //index

				type = m_variant[i].GetType();
				memcpy(pCur, &type, 1); pCur += 1; //type
				
				memcpy(pCur, m_variant[i].m_var, tempSize); pCur += tempSize;

			}
		}
	}

	*pSizeOut = totalSize;
	return pDest;
}

void send_varlist(ENetPeer *peer, VariantList vlist, int netid, int delay)
{
	u_int size = 0;
    u_char *pMem = vlist.SerializeToMem(&size, NULL);

    std::vector<u_char> data = compress_state(::state{
        .type = 01, // @note PACKET_CALL_FUNCTION
        .netid = netid,
        .peer_state = 0x08,
		.id = delay,
		.size = size
    });
	u_int pos = data.size(); // @note sizeof(::state)
	data.resize(pos + size);

	memcpy(data.data() + pos, pMem, size);
    delete[] pMem;

    enet_peer_send(peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
}