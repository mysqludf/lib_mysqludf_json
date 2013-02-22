LIB\_MYSQLUDF\_JSON
===================

JSON is an abbreviation of JavaScript Object Notation. JSON uses a subset of the ecmascript (javascript) language to denote javascript data structures (see also RFC 4627). As such, JSON is a text-oriented data format.

JSON is particularly useful as a format for data exchange. It is mainly used in AJAX web applications as an alternative or in addition to XML. As such, JSON offers a few advantages over XML:

 * less verbosity, resulting in a smaller amount of data to transfer
 * directly deserializable to yield the corresponding object structure using the javascript built-in eval() function.

Usually, database data is mapped to the JSON format downstream of the database. For example, PHP offers an extension to convert PHP data structures to JSON. The disadvantage of that approach is that it usually requires one to iterate through the resultset to create an object structure out of it before that object structure can be encoded into the JSON format. Also, JSON encoding using an UDF is much faster.

###General notes of usage###

There are a few things to keep in mind when using these UDF's. The JSON UDFs are designed for ease of use. They were designed with the assumption in mind that in many cases, only a simple and straightforward mapping is required. So, the UDFs try to make simple mappings easy and with as little overhead as possible. At the same time the UDFs will still be useful to generate more complicated mappings.

Here's a general overview of general functionality offered by lib\_mysqludf\_json:

* **Variable-length parameterlists allow straightforward mapping**
  - In many cases, a mapping is needed to map row-level data to either a JSON array or a JSON object. Variable argument lists allow a single method invocation to accept a whole list of items that are subsequently mapped to JSON structures.
* **Proper escaping of string data**
  - String values are mapped to JSON strings. These strings are automatically escaped to conform to the JSON syntax. At the same time it is very straightforward to bypass the automatic escaping behaviour when necessary.
* **Consistent NULL handling**
  - Database NULL values are handled according to the datatype. String NULL values map to javascript null, and numeric NULL values map to javascript NaN.
* **Automatic mapping of expressions to JSON object members**
  - Often, when creating JSON objects from relational data, it is perfectly fine to use column names as object member names. In those cases, one need not provide member names explicitly. The functions simple reuse the literal text of the argument expressions. At the same time, it is easy to map members to a custom name.
* **Basic validity checking of JSON member names**
  - Constant object member names are checked to see if they are valid JSON member names.
* **Support for easy nesting of JSON structures**
  - Occassionally, there is a need for nesting JSON objects and/or arrays. Nesting of JSON UDF functions is intuitive and straigtforward. Normally, a string result returned by an inner function would be escaped by the outer function, but this is not the case when nesting the json UDF's.

###Escaping and quoting of string data###

Argument string values are normally automatically mapped to JSON double quoted strings. The string value is escaped to for a valid JSON value. The following characters are escaped by prefixing them with a \ or slash character:

* " - The double quote character is mapped as \"
* linefeed - The linefeed character is mapped to \n
* carriage return - The carriage return character is mapped to \r
* \ - The backslash character is mapped to \

Quoting and escaping can be avoided if the expression that returns the string starts with the prefix json\_ or has an alias starting with that prefix.

###Mapping expressions to Object members###

To make it as easy as possible to map rows to JSON objects, the `json_object` function maps each argument to one member. The expression text is used as member name, and the value as member value. This allows a lot of common cases to be handled without effort.

However, that is not to say that the `json_object` function blindly uses the expression text. With a few exceptions, the expressions are checked to see if they make a valid JSON member name. In some cases, a modification of the expression text is used. Currently modication occurs for qualified names. A qualified column expression like `aTableAlias.columnName` is modified to read `columnName`.

It is always possible to override the automatic member name mapping. This is done by including an alias to the expression inside the function call.


FUNCTIONS
---------

This library lib\_mysqludf\_json aims to offer a complete set of functions to map relational data into JSON format. The following functions are currently supported:

* `json_array` - maps a variable number of arguments to a JSON array.
* `json_members` - maps a variable number of name-value pairs to a list of JSON object members.
* `json_object` - maps a variable number of arguments to a JSON object.
* `json_values` - maps a variable number of arguments to JSON values.

Use `lib_mysqludf_json_info()` to obtain information about the currently installed version of lib\_mysqludf\_json.

###json\_array([arg1,..,argN])###

json\_array takes a variable number of arguments and returns a string that denotes a javascript array with the arguments as members.

#####[arg1,..,argN]#####
A (possibly empty) list of values of any type.

* Numeric types are mapped to decimal numbers.
* Strings, bit and temporal types are mapped to double quoted strings.
* NULL values are handled as follows:
   - For numeric types, NULL is mapped to NaN.
   - For string types, NULL is mapped to null.
* The argumentlist maybe empty, which case an string is returned that denotes an empty array.

#####returns#####
A javascript expression that evaluates to an array.


####Installation####

Place the shared library binary in an appropriate location. Log in to mysql as root or as another user with sufficient privileges, and select any database. Then, create the function using the following DDL statement:

```
CREATE FUNCTION json_array RETURNS STRING SONAME 'lib_mysqludf_json.so';
```

The function will be globally available in all databases.

The deinstall the function, run the following statement:

```
DROP FUNCTION json_array;
```

####Examples####

Create an array of user data:

```
select json_array(
           customer_id
       ,   first_name
       ,   last_name
       ,   last_update
       ) as customer
from   customer 
where  customer_id =1;
    
+------------------------------------------+
| customer                                 |
+------------------------------------------+
| [1,"MARY","SMITH","2006-02-15 04:57:20"] |
+------------------------------------------+
```

To obtain nested arrays, simply nest the function calls:

```
select json_array(
           customer_id
       ,   json_array(
               first_name
           ,   last_name
           )
       ,   last_update
       ) as customer
from   customer 
where  customer_id =1;
    
+--------------------------------------------+
| customer                                   |
+--------------------------------------------+
| [1,["MARY","SMITH"],"2006-02-15 04:57:20"] |
+--------------------------------------------+
```

Note that arrays can be nested freely as described. The string returned by the inner `json_` function is not escaped by the outer `json_` function.
Note that string values are automatically mapped to double-quoted json strings. If need be, special characters are escaped. For example:

```
select json_array('\"\"');
    
+--------------------+
| json_array('\"\"') |
+--------------------+
| ["\"\""]           |
+--------------------+
```

Sometimes one needs to avoid this automatic escaping. This would be the case for example when storing JSON strings directly in the database. If you want to avoid escaping to map a string value as is, use an alias with the prefix json_.

```
select json_array('\"\"' json_);
    
+--------------------------+
| json_array('\"\"' json_) |
+--------------------------+
| [""]                     |
+--------------------------+
```

The alias is not required when the expression itself already has a name that starts with the `json_` prefix. For example, if you have a table with a column called `json_data`, then the value for that column will not be escaped. If that is actually an undesired effect, and you realy want to escape the value after all, simply use an alias with a prefix not equal to `json_`.

###json\_members(name1,value1[,..,..,nameN,valueN])###

This function can be used to turn an arbitrary list of name-value pairs into a list of JSON object members. Normally, one does not need this function directy, as the automatic mapping offered by json\_object is usually sufficient.

One of the things the automatic mapping does not handle well is nesting objects. The json\_members function was designed especially for the purpose of easy nesting of objects, and also to work with array-type object memebers.

#####name1,value1[,..,..,nameN,valueN]#####
A list of name-value pairs. The name is used as a member JSON object member name. The value is used as the value of the member.

#####returns#####
A comma separated list of JSON member names with the associated JSON value.

If the name is a constant expression, a check is performed to see if it yields a valid json member name. In that case, a modification is applied to qualified column names in that the qualifier is stripped from the member name. If the name is not a constant expression, no checking is performed at all. The value of the name argument is simply used as-is as member name, which may lead to syntactically invalid JSON expressions.

####Installation####

Place the shared library binary in an appropriate location. Log in to mysql as root or as another user with sufficient privileges, and select any database. Then, create the function using the following DDL statement:

```
CREATE FUNCTION json_members RETURNS STRING SONAME 'lib_mysqludf_json.so';
```

The function will be globally available in all databases.

The deinstall the function, run the following statement:

```
DROP FUNCTION json_members;
```
 
####Examples####

To obtain nested objects, use this function as an argument for `json_object`

```
select      json_object(
                f.last_update
            ,   json_members(
                    'film'
                ,   json_object(
                        f.film_id
                    ,   f.title
                    ,   f.last_update
                    )
                ,   'category'
                ,   json_object(
                        c.category_id
                    ,   c.name
                    ,   c.last_update
                    )
                )
            ) as film_category
from        film_category fc 
inner join  film f
on          fc.film_id = f.film_id
inner join  category c
on          fc.category_id = c.category_id
where       f.film_id =1;
```
    
yields a string representing the following JSON object (indentation added for readability):

```json
{
    last_update:"2006-02-15 05:03:42"
,   film:{
        "film_id":1
    ,   "title":"ACADEMY DINOSAUR"
    ,   "last_update":"2006-02-15 05:03:42"
    }
,   category:{
        "category_id":6
    ,   "name":"Documentary"
    ,   "last_update":"2006-02-15 04:46:27"
    }
}
```

Note that the result of the inner `json_object` calls is not escaped. That's because `json_members` will not automatically escape values from expressions where the literal expression text has the prefix `json_`. This can pose a problem. If you want to use `json_members` to map a string in column, no escaping is applied if the column happens to have a name that starts with `json_`. In those cases, you can use an alias in the function call:

```
select      json_object(
                json_members(
                    'data'
                ,   json_data data
                )
            )
from        json_data_table
```
 
###json\_object([arg1,..,argN])###

json\_object takes a variable number of arguments, maps them to JSON values and returns a concatenation of these.

#####[arg1,..,argN]#####
A (possibly empty) list of values of any type. The expression text passed as argument is used as member name. This is by design: it makes it very easy to render plain table data as json objects. The expression is checked to verify that it yields a valid javascript identifier.

Sometimes, it is not appropriate to use the expression as member name. In those cases, one can use an alias inside the function call (see examples).

The argument value is the value of the corresponing member.

* Numeric types are mapped to decimal numbers.
* Strings, bit and temporal types are mapped to double quoted strings.
* NULL values are handled as follows:
   - For numeric types, NULL is mapped to NaN.
   - For string types, NULL is mapped to null.
* The argumentlist maybe empty, which case an empty string is returned.

#####returns#####
A javascript expression that evaluates to an javascript value.

###Installation###

Place the shared library binary in an appropriate location. Log in to mysql as root or as another user with sufficient privileges, and select any database. Then, create the function using the following DDL statement:

```
CREATE FUNCTION json_object RETURNS STRING SONAME 'lib_mysqludf_json.so';
```

The function will be globally available in all databases.

The deinstall the function, run the following statement:

```
DROP FUNCTION json_object;
```

###Examples###

Create an object of user data:

```
select json_object(
           customer_id
       ,   first_name
       ,   last_name
       ,   last_update
       ) as customer
from   customer 
where  customer_id =1;
    
+---------------------------------------------------------------------------------------+
| customer                                                                              |
+---------------------------------------------------------------------------------------+
| {customer_id:1,first_name:"MARY",last_name:"SMITH",last_update:"2006-02-15 04:57:20"} |
+---------------------------------------------------------------------------------------+
```

Use the `json_member` function to nest objects:

```
select      json_object(
                f.last_update
            ,   json_members(
                    'film'
                ,   json_object(
                        f.film_id
                    ,   f.title
                    ,   f.last_update
                    )
                ,   'category'
                ,   json_object(
                        c.category_id
                    ,   c.name
                    ,   c.last_update
                    )
                )
            ) as film_category
from        film_category fc 
inner join  film f
on          fc.film_id = f.film_id
inner join  category c
on          fc.category_id = c.category_id
where       f.film_id =1;
```

###json\_values([arg1,..,argN])###

json\_values takes a variable number of arguments and returns a string that denotes a concatenation of javascript objects

#####[arg1,..,argN]#####
A (possibly empty) list of values of any type.

* Numeric types are mapped to decimal numbers.
* Strings, bit and temporal types are mapped to double quoted strings.
* NULL values are handled as follows:
   - For numeric types, NULL is mapped to NaN.
   - For string types, NULL is mapped to null.
* The argumentlist maybe empty, in which case an empty string is returned.

#####returns#####
A javascript value.

####Installation####

Place the shared library binary in an appropriate location. Log in to mysql as root or as another user with sufficient privileges, and select any database. Then, create the function using the following DDL statement:

```
CREATE FUNCTION json_values RETURNS STRING SONAME 'lib_mysqludf_json.so';
```

The function will be globally available in all databases.

The deinstall the function, run the following statement:

```
DROP FUNCTION json_values;
```

####Examples####

Create an javascript value:

```
select json_values('json');

+---------------------+
| json_values('json') |
+---------------------+
| "json"              |
+---------------------+
1 row in set (0.00 sec)
```

