/* 
	lib_mysqludf_json - a library of mysql udfs to map data to JSON format
	Copyright (C) 2007  Roland Bouman 
	web: http://www.xcdsql.org/MySQL/UDF/ 
	email: mysqludfs@gmail.com
	
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
	
*/
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(WIN32)
#define DLLEXP __declspec(dllexport) 
#else
#define DLLEXP
#endif

#ifdef STANDARD
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef __WIN__
typedef unsigned __int64 ulonglong;
typedef __int64 longlong;
#else
typedef unsigned long long ulonglong;
typedef long long longlong;
#endif /*__WIN__*/
#else
#include <my_global.h>
#include <my_sys.h>
#endif
#include <mysql.h>
#include <m_ctype.h>
#include <m_string.h>
#include <stdlib.h>

#include <ctype.h>

#ifdef HAVE_DLOPEN

#define JSON_VALUES 0
#define JSON_ARRAY 1
#define JSON_OBJECT 2
#define JSON_MEMBERS 3
#define LIBVERSION "lib_mysqludf_json version 0.0.2"
#define JSON_RESULT 127
#define JSON_PREFIX "json_"
#define JSON_PREFIX_LENGTH 5
#define JSON_NAN "NaN"
#define JSON_NAN_LENGTH 3
#define JSON_NULL "null"
#define JSON_NULL_LENGTH 4

#ifdef __WIN__
#define HAS_JSON_PREFIX(arg)		(_stricmp(arg,JSON_PREFIX)==0)
#else
#define HAS_JSON_PREFIX(arg)		(strncasecmp(arg,JSON_PREFIX,JSON_PREFIX_LENGTH)==0)
#endif

#ifdef	__cplusplus
extern "C" {
#endif
/**
 * lib_mysqludf_json_info
 */
DLLEXP 
my_bool lib_mysqludf_json_info_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
);

DLLEXP 
void lib_mysqludf_json_info_deinit(
	UDF_INIT *initid
);

DLLEXP 
char* lib_mysqludf_json_info(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char* result
,	unsigned long* length
,	char *is_null
,	char *error
);

/*
 * 	JSON VALUES
 */

DLLEXP 
my_bool json_values_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
);

DLLEXP 
void json_values_deinit(
	UDF_INIT *initid
);

DLLEXP 
char* json_values(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char* result
,	unsigned long* length
,	char *is_null
,	char *error
);

/*
 * 	JSON ARRAY
 */

DLLEXP 
my_bool json_array_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
);

DLLEXP 
void json_array_deinit(
	UDF_INIT *initid
);

DLLEXP 
char* json_array(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *result
,	unsigned long *length
,	char *is_null
,	char *error
);

/*
 * 	JSON OBJECT
 */
DLLEXP 
my_bool json_object_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
);

DLLEXP 
void json_object_deinit(
	UDF_INIT *initid
);

DLLEXP 
char* json_object(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *result
,	unsigned long *length
,	char *is_null
,	char *error
);

/**
 * JSON_MEMBER
 */
DLLEXP 
my_bool json_members_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
);

DLLEXP 
void json_members_deinit(
	UDF_INIT *initid
);

DLLEXP 
char* json_members(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char* result
,	unsigned long* length
,	char *is_null
,	char *error
);
#ifdef	__cplusplus
}
#endif

/*
 * 	JSON utilities
 */
 

/*
 * is_valid_json_member_name
 * 
 * -checks for a valid javascript identifier
 * -modifies qualified names to unqualified names
 * 
 */
my_bool is_valid_json_member_name(
	char* name				//name (identifier)			(in/out)
,	unsigned long* length	//length of name argument	(in/out)
,	char* message			//error message
,	my_bool* status			//error status
){
	int unsigned i=0;
	int unsigned j=0;
	if(*length==0){
		strcpy(
			message
		,	"Invalid json member name - name cannot be empty"
		);		
		(*status) = 1;
	} else {
//This label marks the start of checking an actual identifier
//We distinguish between identifier start chars and identifier chars, 
//so we have a special check for the first character.
//In many cases, our expression will be a qualified column name: table.column
//In these cases we want to return only the column name. 
//So, when we detect the first dot in the expression text, we skip the dot
//And reenter here to pretend the part after the dot is the actual identifier. 
reentry:		
		if(!	//if not a valid identifier start char
			(	(name[i]>='A' && name[i]<='Z')
			||	(name[i]>='a' && name[i]<='z')
			||	 name[i]=='_'
			||	 name[i]=='$')
		){	
			strcpy(
				message
			,	"Invalid json member name - name cannot start with '"
			);
			message[51] = name[i];
			message[52] = '\'';
			message[53] = '\0';
			(*status) = 1;
		} else {
			if (j!=i){				//in case of reading the unqualified identifier
				name[j] = name[i];	//copy the first character
			}
			for(i++,j++; i<*length; i++,j++){
				if(name[i] <= ' '){	//quick and dirty whitespace check - marks the end of the expression 
					*length = j;	//cut off the name here
					break;
				} else if(!	//if not an ordinary identifier char, 
					(	(name[i]>='A' && name[i]<='Z')
					||	(name[i]>='a' && name[i]<='z')
					||	(name[i]>='0' && name[i]<='9')
					||	name[i]=='_'
					||	name[i]=='$')
				) {
					//check for dot, if we find one we are looking at a qualified name
					//for a qualified name, we unqualify it, taking the bit beyond the dot. 
					if (name[i]=='.'	//found a dot
					&&	j==i			//and this is the first dot
					){
						j = 0;				//start writing at the start again
						i++;				//look for the part beyond the dot.						
goto reentry;
					} else {
						// either a dot beyond the first dot or not an identifier char alltogether.
						strcpy(
							message
						,	"Invalid json member name - name cannot contain '"
						);
						message[48] = name[i];
						message[49] = '\'';
						message[50] = '\0';
						(*status) = 1;
						break;
					}
				} else {
					if (j!=i){
						name[j] = name[i];
					} 
				}
			}			
			*length = j;
		}
	}
	return *status;
} 
/*
 * prepare_json
 */
my_bool prepare_json(
 	UDF_ARGS *args
,	char *message
,	char type
,	char* arg_types_ptr
,	unsigned long* scalar_result_length_ptr
){
	my_bool status;
	unsigned int i;
	unsigned long string_buffer_length = 0;
	unsigned long other_buffer_length = 0;
	
	if(	type==JSON_OBJECT
	||	type==JSON_ARRAY){
		//add 2 for the opening and closing delimiters
		other_buffer_length += 2;
	}
	for(i=0; i<args->arg_count; i++){
		if(type==JSON_OBJECT){
			if(is_valid_json_member_name(
				args->attributes[i]
			,	&args->attribute_lengths[i]
			,	message
			,	&status
			)!=0){
				return 1;
			}
			//add member name, colon and comma, and enclosing double quotes
			other_buffer_length += args->attribute_lengths[i] + 1 + 1 + 2; 
		} else if (type==JSON_ARRAY){
			//add comma
			other_buffer_length += 1;
		}
		if(type==JSON_MEMBERS
		&&((i%2)==0)){
			if(args->arg_type[i]!=STRING_RESULT){
				//exit with an error if it is not a string type
				strcpy(
					message
				,	"Member name must be a string type."
				);
				return 1;
			} else if(args->args[i]!=NULL){
				//if it is a constant, check if it is a valid member name
				if(is_valid_json_member_name(
					args->args[i]
				,	&args->lengths[i]
				,	message
				,	&status
				)!=0){
					return 1;
				}
			}
			//add member name, colon and comma, and enclosing double quotes
			other_buffer_length += args->attribute_lengths[i] + 1 + 1 + 2; 
		} else {
			if (args->arg_type[i]==STRING_RESULT){
				if (HAS_JSON_PREFIX(args->args[0])){
					arg_types_ptr[i] = JSON_RESULT;
					if(args->lengths[i]<JSON_NULL_LENGTH){
						other_buffer_length += JSON_NULL_LENGTH;
					} else{
						other_buffer_length += args->lengths[i]; 
					} 
				} else {
					arg_types_ptr[i] = args->arg_type[i];
					if(args->lengths[i]<JSON_NULL_LENGTH){
						other_buffer_length += JSON_NULL_LENGTH;
					} else{
						//string buffer length is attribute length 
						//plus opening and closing delimiters.
						//however, we need to take escapig into account
						//in a worst case every character is escaped to a 
						//2 character escape sequence. 
						//So, by adding only one delimiter and multiplying 
						//all string length at the end, we obtain the 
						//maximum length for the string.
						string_buffer_length += args->lengths[i] + 1; 
					} 
				}				
				/* mark as JSON */
			} else {
				/* copy the type */
				arg_types_ptr[i] = args->arg_type[i];
				if(args->lengths[i]<JSON_NAN_LENGTH){
					other_buffer_length += JSON_NAN_LENGTH;
				} else{
					other_buffer_length += args->lengths[i]; 
				}
			}
		}
	}
	//calculate final result length
	*scalar_result_length_ptr = 
		other_buffer_length 		
	+	2 * string_buffer_length	//2 * string 
	;  
	return 0;
}
my_bool json_init2(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
,	int unsigned type
){
	my_bool status = 0;
	unsigned long scalar_result_length = 0;
	char* arg_types = NULL;

	if(!(arg_types = (char *)malloc(args->arg_count))){
		/* Whoops! Pity but we're most likely out of memory */
		strcpy(
			message
		,	"Could not allocate memory (udf: json_init)"
		);		
		return 1;
	}
	if(prepare_json(
	 	args
	,	message
	,	type
	,	arg_types
	,	&scalar_result_length
	)==0){
		if ((initid->ptr = malloc(
			args->arg_count
		+	scalar_result_length
		))){
			memcpy(
				initid->ptr
			,	arg_types
			,	args->arg_count
			);
		} else {
			strcpy(
				message
			,	"Could not allocate memory"
			);		
			status = 1;
		}		
	}
	free(arg_types);
	return status;
}

/*
 * 	json_init
 * 
 * 	xxx_init function for json_array and json_object
 * 
 * 	The main job of this function is to allocate memory to 
 *  repeatedly render the desired JSON array or object.
 * 
 * 	This function does not check any parameters types or counts
 *  This is by design: JSON objects and arrays maybe empty,
 *  and in some rare circumstances (dynamic SQL) it may be 
 *  appropriate to generate empty results.
 * 
 *  NULL handling:
 * 		SQL NULL values are rendered as java script null values
 *  limitations:
 * 		dates and times are rendered as ordinary strings  
 */
my_bool json_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
,	int unsigned type
){
	my_bool status = 0;
	/* 
	 * buffer_size: first used to calculate the fixed length buffer.
	 * initial value 2 is computed as follows:
	 * 	json array or object is delimited by [] and {} respectively,
	 *  requiring 2 characters
	 */
	int unsigned buffer_size = 2;
	/*
	 * string buffer size: used to calculate all required buffer size for
	 * STRING_RESULT arguments. This is calculated separately, because
	 * strings need escaping (see escape_json_string). 
	 * and an additional calculation must be applied to the raw maximum 
	 * argument lengths in order to account for extra characters inserted
	 * by the escaping process.
	 */ 
	int unsigned string_buffer_size = 0;
	int unsigned i; 
	/*
	 * We use arg_types in order to be able to extend the number of argument types
	 * We need that to mark those arguments that we know are already json strings
	 * It is important to know that, because arguments that are already json strings
	 * should be escaped only once. 
	 */
	char* arg_types = NULL;
	if(!(arg_types = (char *)malloc(args->arg_count))){
		/* Whoops! Pity but we're most likely out of memory */
		strcpy(
			message
		,	"Could not allocate memory (udf: json_init)"
		);		
		status = 1;
	} else {
		/* loop over all arguments */
		for(
			i=0
		;	i < args->arg_count
		;	i++
			/* increment buffer size to account for the separator
			 * members in the json array or object are separated by 
			 * a single comma, which is why we need to add 1 byte of buffer.
			*/
		,	buffer_size++
		){
			/*
			 * Check if the argument is a json string
			 * We assume that any expression that has a "json_" (case insensitive)
			 * prefix, is a json string.
			 **/
			if (args->arg_type[i]==STRING_RESULT
			&&	HAS_JSON_PREFIX(args->attributes[i])
			){
				/* mark as JSON */
				arg_types[i]=JSON_RESULT;
			} else {
				/* copy the type */
				arg_types[i]=args->arg_type[i];
			}
			/* calculate the maximum required buffer for this argument */
			if(type==JSON_OBJECT
			&&	arg_types[i]!=JSON_RESULT
			){
				/*
				 * Check if this is a vailid member name
				 **/
				if(is_valid_json_member_name(
					args->attributes[i]
				,	&args->attribute_lengths[i]
				,	message
				,	&status
				)==1){
					break;
				}
				/* For a json object, add length for the name + 1 + 2
				 * the addition 1 is for the colon which separates 
				 * the member name from its value as in
				 * 
				 * name:<value>
                 *
				 * the addition 2 is to quote the member name as in
				 * 
				 * "identifier":<value>
				 */
				buffer_size += args->attribute_lengths[i] + 1 + 2;
			}
			switch(arg_types[i]){
				/* In all these cases, allocate the advocated maximum length
				 */
				case DECIMAL_RESULT:
				case INT_RESULT:
				case REAL_RESULT:
					/*	allocate maximum length
					 *	beware of NULL values though - length may be 0, 
					 *	then we need to allocate 3 to render NaN
					 * */				
					buffer_size += args->lengths[i]<JSON_NAN_LENGTH
					?	JSON_NAN_LENGTH
					:	args->lengths[i]
					; 
					break;
				case JSON_RESULT:
					buffer_size += args->lengths[i]; 
					buffer_size += args->lengths[i]<JSON_NULL_LENGTH
					?	JSON_NULL_LENGTH
					:	args->lengths[i]
					; 
					break;
				case STRING_RESULT:
				/* For strings, allocate the advocated maximum length
				 * and add 1 to account for the fact that strings are quoted.
				 * Here, we need to add only 1 for quoting and not 2 because
				 * the entire string_buffer_size is multiplied by 2 anyway.
				 * This multiplication takes care of the fact that in a worst
				 * case scenario, each character might be escaped by escape_json_string
				 * The multiplicaton conveniently accounts for the closing quote character. 
				 */
					string_buffer_size += 
					(	args->lengths[i]<JSON_NULL_LENGTH
					?	JSON_NULL_LENGTH
					:	args->lengths[i]
					)	+ 1
					;
					break;
			}
		}
		/*
		 * status could have been changed by the call to is_valid_json_member_name
		 * If it is 0, everything is still ok - it will be 1 when we have a bad member name. 
		 */
		if (status==0){	
			/* Perform the actual allocation of memory */
			if ((initid->ptr = malloc(
				args->arg_count
			+	buffer_size
			+	string_buffer_size*2
			))==NULL){
				/* Whoops! Pity but we're most likely out of memory */
				strcpy(
					message
				,	"Could not allocate memory (udf: json_init)"
				);		
				status = 1;
			} else {
				/*Copy our custom list of argument types to the beginning of our working buffer*/
				memcpy(
					initid->ptr
				,	arg_types
				,	args->arg_count
				);
				/* Ok, out of the woods */
				status = 0;
			}
		}
		/*free our list of argument types*/
		if (arg_types!=NULL){
			free(arg_types);
		}
	}
	return status;
}
/*
 * 	json_deinit
 * 
 * 	xxx_deinit function for json_array and json_object
 */
void json_deinit(
	UDF_INIT *initid
){	
	/* If we allocated memory, free it */
	if (initid->ptr!=NULL){
		free(initid->ptr);
	}
}


/*
 * 	write_json_value
 * 
 *	reusable helper function to write a single value as JSON   
 */
void write_json_value(
	char* value				//the value
,	unsigned long length	//the length as reported by args->lengths
,	char type				//the type as reported by args->arg_type
,	char** buffer_ptr		//a pointer to the buffer. This valus is updated to reflect the actual length written
){
	unsigned long i;
	if(value==NULL){				//check if we are writing a NULL value
		switch(type){				//for objects and strings maps to javascript null
			case JSON_RESULT:
			case STRING_RESULT:
				memcpy(
					*buffer_ptr
				,	"null"
				,	4
				);
				*buffer_ptr += 4;
				break;
			case DECIMAL_RESULT:	//for numbers map to javascript NaN
			case REAL_RESULT:
			case INT_RESULT:
				memcpy(
					*buffer_ptr
				,	"NaN"
				,	3
				);
				*buffer_ptr += 3;
				break;
		}
	} else {						//not NULL write a real value.
		switch(type){
			case JSON_RESULT:		//write as-is for decimal and json values
			case DECIMAL_RESULT:
				memcpy(
					*buffer_ptr
				,	value
				,	length
				);
				*buffer_ptr += length;
				break;
			case INT_RESULT:		//write int value
				sprintf(
					*buffer_ptr
				,	"%lld"
				,	*((longlong *)value)
				);
				*buffer_ptr+=strlen(*buffer_ptr);
				break;
			case REAL_RESULT:		//write float value
				sprintf(
					*buffer_ptr
				,	"%f"
				,	*((double *)value)
				);
				*buffer_ptr+=strlen(*buffer_ptr);
				break;
			case STRING_RESULT:		//write string value
				//add opening quote
				*(*buffer_ptr)='"';	
				*buffer_ptr += 1;				
				//loop through the string to escape metacharacters 
				for(i=0; i<length; i++){
					switch (value[i]){
						case '\n':							
							*(*buffer_ptr+1) = 'n';
							*(*buffer_ptr)='\\';
							*buffer_ptr+=2;
							break;
						case '\r':
							*(*buffer_ptr+1) = 'r';
							*(*buffer_ptr)='\\';
							*buffer_ptr+=2;
							break;
						case '"':
						case '\\':
							*(*buffer_ptr+1) = value[i];
							*(*buffer_ptr)='\\';
							*buffer_ptr+=2;
							break;							
						default:	//by default, copy the character as is
							*(*buffer_ptr)=value[i];
							*buffer_ptr += 1;
					}
				}
				//closing quote
				*(*buffer_ptr)='"';
				*buffer_ptr += 1;
				break;
		}
	}
}

char* json(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *result
,	unsigned long *length
,	char *is_null
,	char *error
,	char unsigned type
){
	char* arg_types = initid->ptr;					//first args->arg_count bytes is type info
	char* buffer = initid->ptr + args->arg_count;	//beyond type info is the buffer for the result
	char* start = buffer;							//remember start of result buffer
	//char** buffer_ptr = &buffer;					

	unsigned long i;
	switch(type){						//add opening delimiter
		case JSON_ARRAY:
			*buffer = '[';
			buffer++;
			break;
		case JSON_OBJECT:
			*buffer = '{';
			buffer++;
			break;
		default:
			//do nothing, no delimiters
			break;
	}
	for (i=0; i<args->arg_count; i++){	//loop over the arguments
		switch (type){
			case JSON_MEMBERS:
				if((i%2)==0){
					*buffer = '"';
					buffer++;
					memcpy(							
						buffer
					,	args->args[i]
					,	args->lengths[i]
					);
					buffer += args->lengths[i];
					*buffer = '"';
					buffer++;
					*buffer = ':';
					buffer++;				
					continue;
				} else {
					break;
				}
			case JSON_OBJECT:
				if(arg_types[i]!=JSON_RESULT){
					*buffer = '"';
					buffer++;
					memcpy(							
						buffer
					,	args->attributes[i]
					,	args->attribute_lengths[i]
					);
					buffer += args->attribute_lengths[i];
					*buffer = '"';
					buffer++;
					*buffer = ':';
					buffer++;
				}
				break;
		}
		//write the argument value; this might be an array or object member;
		write_json_value(
			args->args[i]
		,	args->lengths[i]
		,	arg_types[i]
		,	&buffer
		);
		if (type!=JSON_VALUES){
			*buffer = ',';
			buffer++;
		}
	}	
	if (args->arg_count!=0
	&&	type!=JSON_VALUES){
		//adjust length, overwrite last comma.
		buffer--;
	}
	//add closing delimiter
	switch(type){
		case JSON_ARRAY:
			*buffer = ']';
			buffer++;
			break;
		case JSON_OBJECT:
			*buffer = '}';
			buffer++;
		default:
			break;
	}
	//calculate the lenght of the entire result string
	*length = buffer - start;
	//return the begin of the result buffer.
	return start;
}

/**
 * lib_mysqludf_json_info
 */
my_bool lib_mysqludf_json_info_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
){
	my_bool status;
	if(args->arg_count!=0){
		strcpy(
			message
		,	"No arguments allowed (udf: lib_mysqludf_json_info)"
		);
		status = 1;
	} else {
		status = 0;
	}
	return status;
}
void lib_mysqludf_json_info_deinit(
	UDF_INIT *initid
){
}
char* lib_mysqludf_json_info(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char* result
,	unsigned long* length
,	char *is_null
,	char *error
){
	strcpy(result,LIBVERSION);
	*length = strlen(LIBVERSION);
	return result;
}
/*
 * 	JSON VALUES
 */
my_bool json_values_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
){	
	return json_init(
		initid
	,	args
	,	message
	,	JSON_VALUES
	);
}
void json_values_deinit(
	UDF_INIT *initid
){	
	json_deinit(
		initid
	);
}
char* json_values(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *result
,	unsigned long *length
,	char *is_null
,	char *error
){
	return json(
		initid
	,	args
	,	result
	,	length
	,	is_null
	,	error
	,	JSON_VALUES
	);
}
/*
 * 	JSON ARRAY
 */
my_bool json_array_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
){
	return json_init(
		initid
	,	args
	,	message
	,	JSON_ARRAY
	);
}
void json_array_deinit(
	UDF_INIT *initid
){	
	json_deinit(
		initid
	);
}
char* json_array(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *result
,	unsigned long *length
,	char *is_null
,	char *error
){
	return json(
		initid
	,	args
	,	result
	,	length
	,	is_null
	,	error
	,	JSON_ARRAY
	);
}

/*
 * 	JSON OBJECT
 */
my_bool json_object_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
){
	return json_init(
		initid
	,	args
	,	message
	,	JSON_OBJECT
	);
}
void json_object_deinit(
	UDF_INIT *initid
){	
	json_deinit(
		initid
	);
}
char* json_object(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *result
,	unsigned long *length
,	char *is_null
,	char *error
){
	return json(
		initid
	,	args
	,	result
	,	length
	,	is_null
	,	error
	,	JSON_OBJECT
	);
}
/**
 * JSON_MEMBERS
 * */
my_bool json_members_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
){
	my_bool status = 0;
	int unsigned i;
	unsigned char* arg_types = NULL;
	long unsigned buffer_size = 0;
	long unsigned string_buffer_size = 0;
	if ((args->arg_count < 2) 
	||	(args->arg_count % 2)!=0
	){
		strcpy(
			message
		,	"Only non-zero even number of arguments allowed (udf: json_members_init)"
		);
		status = 1;
	} else if(
		(arg_types = (unsigned char *)malloc(args->arg_count))==NULL
	){
		/* Whoops! Pity but we're most likely out of memory */
		strcpy(
			message
		,	"Could not allocate memory (udf: json_members_init)"
		);		
		status = 1;	 
	} else {
		/* loop over the member name arguments */
		for (i=0; i<args->arg_count; i+=2){
			if (args->arg_type[i]!=STRING_RESULT){
				/* must be a string, else it's invalid by definition */
				strcpy(
					message
				,	"String type required for member name (udf: json_members_init)"
				);		
				status = 1;
				break;
			} else if(args->args[i]!=NULL){
				/* 
				 * if it's  a constant, check if it's avalid member name
				 * Basically, we allow variable member names, but we won't check validity
				 * So, it is flexible but you are on your own if it turns out to be a name
				 * that is not a valid js identifier.
				 * */ 
				if(is_valid_json_member_name(
					args->args[i]
				,	&args->lengths[i]
				,	message
				,	&status
				)!=0){
					break;
				}
			}
			/*
			 * Make room for the member name
			 * 
			 * */
			buffer_size += args->lengths[i];
		}
		if (status==0){
			/* loop over the value arguments*/
			for (i=1; i<args->arg_count; i+=2){
				switch (args->arg_type[i]){
					case STRING_RESULT:
						if(HAS_JSON_PREFIX(args->attributes[i])){
							/* mark as JSON */
							arg_types[i]=JSON_RESULT;
							buffer_size += args->lengths[i];
						} else {
							/* copy the type */
							arg_types[i]=args->arg_type[i];
							string_buffer_size += args->lengths[i] + 1;
						}
						break;
					case INT_RESULT:					
					case REAL_RESULT:					
					case DECIMAL_RESULT:					
						arg_types[i]=args->arg_type[i];
						buffer_size += args->lengths[i];
						break;
					case ROW_RESULT:
						abort();
				}
				/*this is for the colon and the comma*/
				buffer_size += 2;
			}
			if ((initid->ptr = malloc(
				(args->arg_count)
			+	buffer_size
			+	(2*string_buffer_size)		
			))==NULL){
				strcpy(
					message
				,	"Could not allocate memory (udf: json_members_init)"
				);		
				status = 1;	 
			} else { 			
				/*Copy our custom list of argument types to the beginning of our working buffer*/
				memcpy(
					initid->ptr
				,	arg_types
				,	(args->arg_count)
				);
				/* Ok, out of the woods */
			}
		}				
	}
	if (arg_types!=NULL){		
		free(arg_types);
	}
	return status;
}
void json_members_deinit(
	UDF_INIT *initid
){
	json_deinit(
		initid
	);
}

char* json_members(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char* result
,	unsigned long* length
,	char *is_null
,	char *error
){
	return json(
		initid
	,	args
	,	result
	,	length
	,	is_null
	,	error
	,	JSON_MEMBERS
	);
}

#endif /* HAVE_DLOPEN */

