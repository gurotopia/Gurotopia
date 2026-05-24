//  ***************************************************************
//  Variant - Creation date: 04/13/2009
//  -------------------------------------------------------------
//  Robinson Technologies Copyright (C) 2009 - All Rights Reserved
//
//  ***************************************************************
//  Programmer(s):  Seth A. Robinson (seth@rtsoft.com)
//  ***************************************************************
#pragma once

#include "Math/vec2.hpp"
#include "Math/vec3.hpp"

class VariantList;

#define C_VAR_SPACE_BYTES 16 //enough to fit a rect

class Variant
{
public:

	enum eType
	{
		TYPE_UNUSED,
		TYPE_FLOAT,
		TYPE_STRING,
		TYPE_VECTOR2,
		TYPE_VECTOR3,
		TYPE_UINT32,
		TYPE_ENTITY,
		TYPE_COMPONENT,
		TYPE_RECT,
		TYPE_INT32
				
	};
	Variant(u_int var) {SetDefaults(); Set(var);}
	Variant(int var) {SetDefaults(); Set(var);}

	Variant(float var) {SetDefaults(); Set(var);}
	Variant(const char *var) {SetDefaults(); Set(std::string(var));} // @note const char* support
	Variant(const std::string &var) {SetDefaults(); Set(var);}
	Variant(float x, float y) {SetDefaults(); Set(x,y);}
	Variant(float x, float y, float z) {SetDefaults(); Set(x,y,z);}
	Variant(const CL_Vec2f &v2) {SetDefaults(); Set(v2);}
	Variant(const CL_Vec3f &v3) {SetDefaults(); Set(v3);}
	
	Variant()
	{
		SetDefaults();
	}
	

	void Set(float var) 
	{
		m_type = TYPE_FLOAT; *((float*)m_var) = var;
	}


	void Set(u_int var) 
	{
		m_type = TYPE_UINT32; *((u_int*)m_var) = var;
	}


	void Set(int var) 
	{
		m_type = TYPE_INT32; *((int*)m_var) = var;
	}


	void Set(std::string const &var);
	std::string & GetString()
	{
		return m_string;
	}
	const std::string & GetString() const
	{
		return m_string;
	}

	void Set(CL_Vec2f const &var) 
	{
		m_type = TYPE_VECTOR2; *((CL_Vec2f*)m_var) = var;
	}
	void Set(float x, float y) 
	{
		Set(CL_Vec2f(x,y));
	}

	void Set(CL_Vec3f const &var) 
	{
		m_type = TYPE_VECTOR3; *((CL_Vec3f*)m_var) = var;
	}
	void Set(float x, float y, float z) 
	{
		Set(CL_Vec3f(x,y,z));
	}

	eType GetType() const  {return m_type;}
	

	Variant(const Variant &v)
	{
		SetDefaults();
		//our special copy won't include the sig crap, that stuff can't be copied
		*this = v;
	}

	friend class VariantList;

private:

	void SetDefaults()
	{
		m_type = TYPE_UNUSED;
	}
	eType m_type;
	
	union
	{
		u_char m_var[C_VAR_SPACE_BYTES]; //large enough so we can use the space for all the datatypes we care about
		
		//don't actually use these, these unions help me look at vars when debugging easier
		float m_as_floats[4];
		u_int m_as_uint32s[4];
		int m_as_int32s[4];
	};
	std::string m_string;

};

//a VariantList holds a group of variants, we pass these when we don't know in advance how many variants we want to use

#define C_MAX_VARIANT_LIST_PARMS 8

/*
//example of memory serialization of a VariantList

VariantList v;
v.Get(0).Set(uint32(42));
v.Get(1).Set("Hey guys");

//save to mem
byte *pData = v.SerializeToMem(&size, NULL);

//load from mem
VariantList b;
b.SerializeFromMem(pData, size);

//display
LogMsg("%s, the answer to life is %d", b.Get(1).GetString().c_str(), b.Get(0).GetUINT32());

//clean up the mem we initted earlier
SAFE_DELETE_ARRAY(pData);
*/

class VariantList
{
	public:

		VariantList(){};

		Variant & Get(int parmNum) {return m_variant[parmNum];}
		VariantList(Variant v0) {m_variant[0] = v0;}
		VariantList(Variant v0, Variant v1) {m_variant[0] = v0; m_variant[1] = v1;}
		VariantList(Variant v0, Variant v1, Variant v2) {m_variant[0] = v0; m_variant[1] = v1; m_variant[2] = v2;}
		VariantList(Variant v0, Variant v1, Variant v2, Variant v3) {m_variant[0] = v0; m_variant[1] = v1; m_variant[2] = v2; m_variant[3] = v3;}
		VariantList(Variant v0, Variant v1, Variant v2, Variant v3, Variant v4) {m_variant[0] = v0; m_variant[1] = v1; m_variant[2] = v2; m_variant[3] = v3; m_variant[4] = v4;}
		VariantList(Variant v0, Variant v1, Variant v2, Variant v3, Variant v4, Variant v5) {m_variant[0] = v0; m_variant[1] = v1; m_variant[2] = v2; m_variant[3] = v3; m_variant[4] = v4;  m_variant[5] = v5;}
		VariantList(Variant v0, Variant v1, Variant v2, Variant v3, Variant v4, Variant v5, Variant v6) {m_variant[0] = v0; m_variant[1] = v1; m_variant[2] = v2; m_variant[3] = v3; m_variant[4] = v4;  m_variant[5] = v5; m_variant[6] = v6;}

		u_char * SerializeToMem(u_int *pSizeOut, u_char *pDest); //pass in NULL for dest and it will new[] the memory itself
		Variant m_variant[C_MAX_VARIANT_LIST_PARMS]; //non-dynamic for speed
};

int GetSizeOfData(Variant::eType type);

void send_varlist(ENetPeer *peer, VariantList vlist, int netid = -1, int delay = 0);