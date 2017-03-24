#include "cmysql.h"
#include "clog.h"
#include "cexception.h"

int CMysqlSkel::InitDB(const char *dbName, const char *user = "root", const char *pass = "" )
{
    {
        CLog            log;
        ostringstream   ost;

        ost.str("");
        ost << "CMysqlSkel::InitDB(db [" << dbName << "], user[" << user << "], password [xxxxxxx]): start";
        
        log.Write(DEBUG, ost.str());
    }
    
    db = mysql_init( NULL );
    if( db == NULL )
    {
        CLog    log;
        
        log.Write(ERROR, "CMysqlSkel::InitDB:ERROR: Failed to allocate memory when init MYSQL\n");
        return(-1);
    }

    db = mysql_real_connect(db, DB_HOST, user, pass, dbName, 0, NULL, CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS);

    if( db == NULL )
    {
        CLog    log;
        
        log.Write(ERROR, "CMysqlSkel::InitDB:ERROR: Failed to connect to MYQSL server (mysql_real_connect return NULL)");
        return(-1);
    }

    {
        CLog            log;
        ostringstream   ost;

        ost.str("");
        ost << "CMysqlSkel::InitDB: end (result = 0[ok])";
        
        log.Write(DEBUG, ost.str());
    }
    return(0);
}

void CMysqlSkel::CloseDB( void ) 
{
    {
        CLog            log;
        ostringstream   ost;

        ost.str("");
        ost << "CMysqlSkel::CloseDB: start";
        
        log.Write(DEBUG, ost.str());
    }

    if( db ) mysql_close( db );

    {
        CLog            log;
        ostringstream   ost;

        ost.str("");
        ost << "CMysqlSkel::CloseDB: end";
        
        log.Write(DEBUG, ost.str());
    }
}

// --- Run Insert query only with autoincrement field
// --- Input: string
// --- Output: 0 - error or no AUTO_INCREMENT_FIELD
//              >0 - success
unsigned long CMysqlSkel::InsertQueryDB(string query)
{
    int             r;
    my_ulonglong    resultAutoIncrement = 0;


    {
        CLog    log;
        log.Write(DEBUG, "MYSQL_RES *CMysqlSkel::InsertQueryDB: start (query [", query, "])");
    }

    r = mysql_query( db, query.c_str());
    if( r )
    {
        CLog    log;
    
        log.Write(ERROR, "MYSQL_RES *CMysqlSkel::InsertQueryDB(string query): ERROR in query ", query, mysql_error(db) );
    }
    else
    {
        if (mysql_store_result(db) == 0 &&
            mysql_field_count(db) == 0 &&
            mysql_insert_id(db) != 0)
        {
            resultAutoIncrement = mysql_insert_id(db);
        }
        else
        {
            CLog    log;
        
            log.Write(ERROR, "MYSQL_RES *CMysqlSkel::InsertQueryDB(string query): ERROR: in query ", query, ": (returned > 0 cols) or (resultSet != NULL) or (table missing AUTO_INCREMENT_FIELD)");
        }
    }    

    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "CMysqlSkel::InsertQueryDB: end (return [" << to_string((unsigned long)resultAutoIncrement) << "])";
        
        log.Write(DEBUG, ost.str());
    }

    return (unsigned long)resultAutoIncrement;
}

// --- Run MySQL query
// --- Input: string
// --- Output:     NULL - resultSet is empty OR error
//             not NULL - resultSet
// --- Remark: no error indication
MYSQL_RES *CMysqlSkel::QueryDB(string query)
{
    MYSQL_RES   *result;
    int         r;

    {
        CLog    log;
        log.Write(DEBUG, "MYSQL_RES *CMysqlSkel::QueryDB(string query): query [", query, "]");
    }
    r = mysql_query( db, query.c_str());
    if( r )
    {
        CLog    log;
    
        log.Write(ERROR, "MYSQL_RES *CMysqlSkel::QueryDB(string query): ERROR in query ", query, mysql_error(db) );
        return NULL;
    }
    else
    {
    	result = mysql_store_result( db );
    	numRows = mysql_affected_rows( db );
    	if( numRows == 0 )
    	{
        	if( result ) mysql_free_result( result );
    	    result = NULL;
    	}
    	else
    	{
    	    if(result)
    	    {
        		numFields = mysql_num_fields( result );
        		fieldsInfo = mysql_fetch_fields( result );
    	    }
    	    else
    	    {
        		numFields = 0;
        		result = NULL;
    	    }
    	}
    }
    return(result);
}

bool CMysqlSkel::isError(void)
{
    return strlen(mysql_error(db));
}

const char *CMysqlSkel::GetErrorMessage(void)
{
    return mysql_error(db);
}

int CMysqlSkel::FieldsIndex(const char *fieldName )
{
    for(unsigned int i = 0; i < numFields; i++ )
	if( strcmp( fieldName, fieldsInfo[i].name ) == 0 )
	    return(i);
    return(-1);
}

void CMysqlSkel::FreeResultSet()
{
    if( resultSet )
    {
    	mysql_free_result( resultSet );
    	resultSet = NULL;
    }
}

char *CMysqlSkel::ResultValue( MYSQL_RES *result, unsigned int row, const char *name )
{
    MYSQL_ROW		fr;
    int	            fi = FieldsIndex( name );

    if(fi < 0)
    {
        CLog    log;
        log.Write(ERROR, "CMysqlSkel::ResultValue: ERROR DB-field [", name, "] does not exist");
        
        throw CException("error db");
    }

    mysql_data_seek(result, row);
    fr = mysql_fetch_row(result);

    if(fr) 
        return(fr[fi]);
    else
    {
        CLog    log;
        log.Write(ERROR, "CMysqlSkel::ResultValue: ERROR error in calling mysql_fetch_row(", name, ")");

        return(NULL);
    }
}

char *CMysqlSkel::ResultValue( MYSQL_RES *result, unsigned int row, unsigned int fi )
{
    MYSQL_ROW       fr;

    if(fi < 0) return(NULL);
    mysql_data_seek(result, row);
    fr = mysql_fetch_row(result);
    if(fr)
    return(fr[fi]);
    else
    {
        CLog    log;
    	char	tmp[10];
    	
    	memset(tmp, 0, sizeof(tmp));
    	snprintf(tmp, 9, "%d", fi);
    	log.Write(ERROR, "CMysqlSkel::ResultValue: ERROR in calling mysql_fetch_row( ", tmp, " )");
    	return(NULL);
    }
}

MYSQL_ROW CMysqlSkel::NextFetch(MYSQL_RES *result)
{
	currentRow = mysql_fetch_row(result);
	if(!currentRow)
	{
        	CLog    log;
        	log.Write(ERROR, "error in calling mysql_fetch_row()");
	        return NULL;
	}
	return currentRow;
}

char *CMysqlSkel::ResultValueFast(unsigned int fi)
{
	if(fi < 0) return(NULL);
	if(currentRow)
        	return(currentRow[fi]);
    	else
    	{
        	CLog    log;
        	char    tmp[10];

        	memset(tmp, 0, sizeof(tmp));
        	snprintf(tmp, 9, "%d", fi);
        	log.Write(ERROR, "error in extracting info from currentRow( ", tmp, " )");
        	return(NULL);
	}
}

int CMysqlSkel::ResultRows( MYSQL_RES *result )
{
    return numRows;
}



CMysql::CMysql()
{
    resultSet = NULL;
}

CMysql::CMysql(const char *dbName, const char *login, const char *pass)
{
    resultSet = NULL;
    InitDB(dbName, login, pass);
}

int CMysql::Connect(const char *dbName, const char *login, const char *pass)
{
    return InitDB(dbName, login, pass);
}

int CMysql::Query(string query)
{
    FreeResultSet();
    resultSet = QueryDB(query);
    if(resultSet != NULL) {
       return AffectedRows();
    }
    return 0;
}

unsigned long CMysql::InsertQuery(string query)
{
    FreeResultSet();
    return InsertQueryDB(query);
}

char *CMysql::Get(int row, const char *name)
{
    if(resultSet)
        return ResultValue(resultSet, row, name);
    return(NULL);
}

char *CMysql::Get(int row, int col)
{
    if(resultSet)
        return ResultValue(resultSet, row, col);
    return(NULL);
}

MYSQL_ROW CMysql::NextRow(void)
{
    return NextFetch(resultSet);
}

char *CMysql::GetFast(int index)
{
    return ResultValueFast(index);
}


int CMysql::CountFields()
{
    return numFields;
}

int CMysql::AffectedRows()
{
    return numRows;
}

CMysql::~CMysql()
{
    FreeResultSet();
    CloseDB();
}

