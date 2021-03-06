#include <string>
#include <string.h>
#include <my_global.h>
#include <my_sys.h>
#include <mysql_com.h>
#include <stdlib.h>
#include "ewah.h"
#include <fstream>
#include "vector"
#include "algorithm"
//#include <sstream>
//#include <roaring.h>
//#include <bitset.h>
extern "C" {
	// get bitmap cardinality
	my_bool bm_count_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	long long bm_count(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
	void bm_count_deinit(UDF_INIT *initid);

	// print bitmap detail
	my_bool bm_detail_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	char * bm_detail(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);
	void bm_detail_deinit(UDF_INIT *initid);

	// generate bitmap
	my_bool bm_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	void bm_clear(UDF_INIT* initid, char* is_null, char *error);
	void bm_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error);
	char * bm(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);
	void bm_deinit(UDF_INIT *initid);

	// bitmap 'and' operate
	my_bool bm_and_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	void bm_and_clear(UDF_INIT* initid, char* is_null, char *error);
	void bm_and_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error);
	char * bm_and(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);
	void bm_and_deinit(UDF_INIT *initid);

	// bitmap 'or' operate
	my_bool bm_or_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	void bm_or_clear(UDF_INIT* initid, char* is_null, char *error);
	void bm_or_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error);
	char * bm_or(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);
	void bm_or_deinit(UDF_INIT *initid);

	// bitmap 'and' operate, then get cardinality
	my_bool bm_and_count_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	void bm_and_count_clear(UDF_INIT* initid, char* is_null, char *error);
	void bm_and_count_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error);
	long long bm_and_count(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
	void bm_and_count_deinit(UDF_INIT *initid);

	// bitmap 'or' operate, then get cardinality
	my_bool bm_or_count_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	void bm_or_count_clear(UDF_INIT* initid, char* is_null, char *error);
	void bm_or_count_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error);
	long long bm_or_count(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
	void bm_or_count_deinit(UDF_INIT *initid);
}

void debug_log(uint32_t i)
{
	std::ofstream f;
	f.open("/tmp/mysql-udf.log",std::ios::app);
	f << i << std::endl;
	f.close();
}
/*
void debug_log(char * str)
{
	std::ofstream f;
	f.open("/tmp/mysql-udf.log",std::ios::app);
	f << str << std::endl;
	f.close();
}
*/
void debug_log(std::string str)
{
	std::ofstream f;
	f.open("/tmp/mysql-udf.log",std::ios::app);
	f << str << std::endl;
	f.close();
}
void tofile(EWAHBoolArray<uint32_t> * bitmap)
{
	std::ofstream f;
	f.open("/tmp/bitmap-demo", std::ios::binary);
	bitmap->write(f);
	f.close();
}

char * string2charArray(std::string str)
{
	char* ca = new char[str.size()+1];
	std::copy(str.begin(), str.end(), ca);
	ca[str.size()] = '\0';
	return ca;
}

void readBitmap(UDF_ARGS *args, EWAHBoolArray<uint32_t> * bitmap)
{
	std::string str(args->args[0], args->lengths[0]);
	std::stringstream ss(str);
	bitmap->read(ss);
}

std::string writeBitmap(EWAHBoolArray<uint32_t> * bitmap)
{
	std::stringstream ss;
	bitmap->write(ss);
	return ss.str();
}

std::string detail(EWAHBoolArray<uint32_t> * bitmap)
{
	std::stringstream ss;
	for (EWAHBoolArray<uint32_t>::const_iterator i = bitmap->begin(); i != bitmap->end();) {
		ss << *i;
  	++i; 
		if (i != bitmap->end())
			ss << ","; 
	} 
	return ss.str();
}

my_bool bm_count_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	if (args->arg_count != 1) {
		strcpy(message, "bm_count must have exaclty one argument");
		return 1;
	}
  args->arg_type[0] = STRING_RESULT;
	initid->maybe_null = 0;
	initid->ptr = (char *) new EWAHBoolArray<uint32_t>;
	return 0;
}

long long bm_count(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
	if (args->args[0])
	{
		EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
		readBitmap(args, bitmap);
		return bitmap->numberOfOnes();
	}
	return 0;
}

void bm_count_deinit(UDF_INIT *initid)
{
	delete initid->ptr;
}

// ==================================================================
my_bool bm_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	if (args->arg_count != 1) {
    strcpy(message, "bm must have exaclty one argument");
    return 1;
  }
  args->arg_type[0] = INT_RESULT;
  initid->maybe_null = 1;
	initid->ptr = (char *) new std::vector<uint32_t>; 
	return 0; 
}

void bm_clear(UDF_INIT* initid, char* is_null, char *error)
{

	std::vector<uint32_t> * v = (std::vector<uint32_t> *)initid->ptr;
	v->clear();
}

void bm_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error)
{
	if (args->args[0])
	{
		uint32_t t = *((int*) args->args[0]);
		std::vector<uint32_t> * v = (std::vector<uint32_t> *)initid->ptr;
		v->push_back(t);
	}
}

char * bm(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	std::vector<uint32_t> * v = (std::vector<uint32_t> *)initid->ptr;
	sort(v->begin(), v->end());
	EWAHBoolArray<uint32_t> * bitmap = new EWAHBoolArray<uint32_t>;
	uint32_t i = 0;
  for(; i<v->size(); i++)
  {
		bitmap->set((*v)[i]);
  }
	std::string s = detail(bitmap);
	std::string str = writeBitmap(bitmap);
	memcpy(result, str.c_str(), str.length()+1);
	*length = str.length()+1;
	delete bitmap;
	return result;
}

void bm_deinit(UDF_INIT *initid)
{
	delete initid->ptr;
}

// ==================================================================
my_bool bm_detail_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	if (args->arg_count != 1) {
    strcpy(message, "bm_detail must have exaclty one argument");
    return 1;
  }
  args->arg_type[0] = STRING_RESULT;
  initid->maybe_null = 1;
	initid->ptr = (char *) new EWAHBoolArray<uint32_t>;
	return 0;
}
char * bm_detail(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	std::string str; 
	if (args->args[0])
	{
		EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
		readBitmap(args, bitmap);
		str = detail(bitmap);
	} else {
		str = "";
	}
	memcpy(result, str.c_str(), str.length()+1);
	*length = str.length()+1;
	return result;
}
void bm_detail_deinit(UDF_INIT *initid)
{
	delete initid->ptr;
}

// ==================================================================
my_bool bm_and_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	if (args->arg_count != 1) {
		strcpy(message, "bm_and must have exaclty one argument");
		return 1;
	}
  args->arg_type[0] = STRING_RESULT;
	initid->maybe_null = 0;
	initid->ptr = NULL; 
	return 0;

}
void bm_and_clear(UDF_INIT* initid, char* is_null, char *error)
{
	if (initid->ptr != NULL)
	{
		EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
		bitmap->reset();
	}
}
void bm_and_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error)
{
	if (args->args[0])
	{	
		EWAHBoolArray<uint32_t> * curr = new EWAHBoolArray<uint32_t>;
		readBitmap(args, curr); 
		if (initid->ptr == NULL)
		{
			initid->ptr = (char *) curr;
			return;	
		}
		EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
		EWAHBoolArray<uint32_t> * tmp = new EWAHBoolArray<uint32_t>;
		bitmap->logicaland(*curr, *tmp);
		delete bitmap;
		initid->ptr = (char *) tmp;
		delete curr;
	}
}
char * bm_and(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
	std::string str = writeBitmap(bitmap);
	memcpy(result, str.c_str(), str.length()+1);
	*length = str.length()+1;
	return result;
}
void bm_and_deinit(UDF_INIT *initid)
{
	delete initid->ptr;
}

// ==================================================================
my_bool bm_or_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	if (args->arg_count != 1) {
		strcpy(message, "bm_or must have exaclty one argument");
		return 1;
	}
  args->arg_type[0] = STRING_RESULT;
	initid->maybe_null = 0;
	initid->ptr = (char *) new EWAHBoolArray<uint32_t>;
	return 0;

}
void bm_or_clear(UDF_INIT* initid, char* is_null, char *error)
{
	EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
	bitmap->reset();
}
void bm_or_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error)
{
	if (args->args[0])
	{	
		EWAHBoolArray<uint32_t> * curr = new EWAHBoolArray<uint32_t>;
		readBitmap(args, curr); 
		EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
		EWAHBoolArray<uint32_t> * tmp = new EWAHBoolArray<uint32_t>;
		bitmap->logicalor(*curr, *tmp);
		delete bitmap;
		initid->ptr = (char *) tmp;
		delete curr;
	}
}
char * bm_or(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
	std::string str = writeBitmap(bitmap);
	memcpy(result, str.c_str(), str.length()+1);
	*length = str.length()+1;
	return result;
}
void bm_or_deinit(UDF_INIT *initid)
{
	delete initid->ptr;
}
// ==================================================================
my_bool bm_and_count_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	if (args->arg_count != 1) {
		strcpy(message, "bm_and_count must have exaclty one argument");
		return 1;
	}
  args->arg_type[0] = STRING_RESULT;
	initid->maybe_null = 0;
	initid->ptr = NULL; 
	return 0;

}
void bm_and_count_clear(UDF_INIT* initid, char* is_null, char *error)
{
	if (initid->ptr != NULL)
	{
		EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
		bitmap->reset();
	}
}
void bm_and_count_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error)
{
	if (args->args[0])
	{	
		EWAHBoolArray<uint32_t> * curr = new EWAHBoolArray<uint32_t>;
		readBitmap(args, curr); 
		if (initid->ptr == NULL)
		{
			initid->ptr = (char *) curr;
			return;	
		}
		EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
		EWAHBoolArray<uint32_t> * tmp = new EWAHBoolArray<uint32_t>;
		bitmap->logicaland(*curr, *tmp);
		delete bitmap;
		initid->ptr = (char *) tmp;
		delete curr;
	}
}
long long bm_and_count(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
	EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
	return bitmap->numberOfOnes();
}
void bm_and_count_deinit(UDF_INIT *initid)
{
	delete initid->ptr;
}

// ==================================================================
my_bool bm_or_count_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	if (args->arg_count != 1) {
		strcpy(message, "bm_or_count must have exaclty one argument");
		return 1;
	}
  args->arg_type[0] = STRING_RESULT;
	initid->maybe_null = 0;
	initid->ptr = (char *) new EWAHBoolArray<uint32_t>;
	return 0;

}
void bm_or_count_clear(UDF_INIT* initid, char* is_null, char *error)
{
	EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
	bitmap->reset();
}
void bm_or_count_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error)
{
	if (args->args[0])
	{	
		EWAHBoolArray<uint32_t> * curr = new EWAHBoolArray<uint32_t>;
		readBitmap(args, curr); 
		EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
		EWAHBoolArray<uint32_t> * tmp = new EWAHBoolArray<uint32_t>;
		bitmap->logicalor(*curr, *tmp);
		delete bitmap;
		initid->ptr = (char *) tmp;
		delete curr;
	}
}
long long bm_or_count(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
	EWAHBoolArray<uint32_t> * bitmap = (EWAHBoolArray<uint32_t> *)initid->ptr;
	return bitmap->numberOfOnes();
}
void bm_or_count_deinit(UDF_INIT *initid)
{
	delete initid->ptr;
}

