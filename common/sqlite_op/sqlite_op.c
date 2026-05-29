#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "sqlite_op.h"
#include "tool.h"

#define printf_err_log printf

#define DB_OPEN_CHECK(open_target) (open_target == DB_OPEN_TARGET)

int check_sqlite_exist(char* file_name)
{
	int res;
	if(!file_name)
	{
		return EXIT_FAILURE;
	}

	res = access(file_name, F_OK);
	return (res) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int open_sqlite(sqlite_db* db, char* file_name)
{
	int res;
	int open_target;
	int new_file_name_len;
	open_target = db->open_target;

	if(DB_OPEN_CHECK(open_target))
	{
		sqlite3_close(db->database);
	}
	else
	{
		if(!file_name && db->file_name[0] == '\0')
		{
			printf_err_log("Error!!! database couldnt open! not support database file name\n");
		}
	}

	if(file_name)
	{
		memset(db->file_name, 0, SQLITE_PATH_MAX_LEN);
		new_file_name_len = strlen(file_name);
		new_file_name_len = (new_file_name_len >= STRING_TRUE_LEN(SQLITE_PATH_MAX_LEN))
							? STRING_TRUE_LEN(SQLITE_PATH_MAX_LEN)
							: new_file_name_len;
		memcpy(db->file_name, file_name, new_file_name_len);
	}

	db->database = NULL;
	res = sqlite3_open(db->file_name, &db->database);
	if(res)
	{
		printf_err_log("Error!!! database couldnt open! %s\n", sqlite3_errmsg(db->database));
		sqlite3_close(db->database);
		res = EXIT_FAILURE;
	}

	return res;
}

int close_sqlite(sqlite_db* db)
{
	int res;
	if(!db->database)
	{
		return EXIT_FAILURE;
	}
	res = sqlite3_close(db->database);
	return (res) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int sqlite_exec(sqlite_db* db, char* sql)
{
	int res;
	char* errmsg = NULL;

	res = sqlite3_exec(db->database, sql, NULL, NULL, &errmsg);
	if(res)
	{
		printf_err_log("Error!!! database exec failed!\n");
		printf_err_log("******** command: %s\n", sql);
		printf_err_log("******** errmsg: %s\n", errmsg);
		res = EXIT_FAILURE;
	}
	return res;
}

int sqlite_get_data(sqlite_db* db, char* sql, char*** data, int* data_row, int* data_col)
{
	int res;
	char* errmsg = NULL;

	res = sqlite3_get_table(db->database, sql, data, data_row, data_col, &errmsg);
	if(res != SQLITE_OK)
	{
		printf_err_log("Error!!! database get data failed!\n");
		printf_err_log("******** command: %s\n", sql);
		printf_err_log("******** errmsg: %s\n", errmsg);
		res = EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int sqlite_get_data_no_err(sqlite_db* db, char* sql, char*** data, int* data_row, int* data_col)
{
	int res;
	char* errmsg = NULL;

	res = sqlite3_get_table(db->database, sql, data, data_row, data_col, &errmsg);

	return (res == SQLITE_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int sqlite_get_data_out_title(sqlite_db* db, char* sql, char*** data, int* data_row, int* data_col)
{
	int res;
	int i;
	char** data_no_title = NULL;
	int data_row_ori;
	int data_col_ori;
	int total_size;
	int ori_index;
	int ori_data_len;

	char* errmsg = NULL;

	res = sqlite3_get_table(db->database, sql, data, data_row, data_col, &errmsg);
	if(res != SQLITE_OK)
	{
		printf_err_log("Error!!! database get data failed!\n");
		printf_err_log("******** command: %s\n", sql);
		printf_err_log("******** errmsg: %s\n", errmsg);
		res = EXIT_FAILURE;
	}

	data_row_ori = data_row[0];
	data_col_ori = data_col[0];

	total_size = data_row_ori * data_col_ori;

	data_no_title = (char**)calloc(total_size, sizeof(char*));
	ori_index = data_col_ori;
	for(i = 0; i < total_size; ++i)
	{
		ori_data_len = strlen(data[0][ori_index]);
		data_no_title[i] = (char*)calloc(ori_data_len + 4, sizeof(char));
		strcpy(data_no_title[i], data[0][ori_index]);
		++ori_index;
	}

	sqlite3_free_table(data[0]);
	data[0] = data_no_title;

	return EXIT_SUCCESS;
}

/*
 * @brief: 得到一个数据库的一个表的最新插入的行数 对应的 id
 *		   这个和sqlite3的特性有关, 利用了sqlite3 的 rowid 如果声明了自增的id(int) 那么 rowid就是id
 *		   可以 SELECT last_insert_rowid() FROM [table] limit 1 获得一行数据,那个数据就是rowid
 *		   比如 新加入了 id=8 在 表 abc 中
 *		   那么 SELECT last_insert_rowid() FROM abc limit 1 得到的数据就是
 *		   last_insert_rowid()
 *		   1
 *		   此函数存在申请内存的操作, 调用此函数的函数请自行释放 id 参数申请到的内存
 * @param[in]:参数1(db): sqlite_op定义的结构体 里面包含了sqlite3的指针 用于指定要操作的数据库
 * @param[in]:参数2(table_name): 要查询的数据库的名字
 * @param[out]:参数3(id): 一个 char* 空指针的 地址 用于保存查到的结果 记得此函数调用要 free(*id) 因为这里面会存在 申请内存
 * @return: 成功查询并写入id[0] 返回 EXIT_SUCCESS 否则返回 EXIT_FAILURE;
 */
int sqlite_get_last_id(sqlite_db* db, char* table_name, char** id)
{
	int res;
	char** data = NULL;
	int row;
	int col;
	char sql[80] = {0};

	if(!id || id[0])
	{
		return EXIT_FAILURE;
	}

	sprintf(sql, "SELECT last_insert_rowid() FROM %s limit 1", table_name);

	res = sqlite_get_data(db, sql, &data, &row, &col);
	IF_RES_FAIL_RETURN;

	if(!(row == 1 && col == 1))
	{
		free_data(data);
		return EXIT_FAILURE;
	}

	id[0] = (char*)calloc(32, sizeof(char));

	strcpy(id[0], data[1]);

	free_data(data);

	return EXIT_SUCCESS;
}

/*
 * @brief: 清除单个表格的内容
 * @param[in]: 参数1(db): 操作的数据库信息数据结构指针
 * @param[in]: 参数2(table_name): 操作要删除的表名字
 * @return: 成功消除 返回 EXIT_SUCCESS 否则 返回 EXIT_FAILURE
 */
int truncate_table(sqlite_db* db, char* table_name)
{
	int res;
	char sql[128] = {0};
	sprintf(sql, "DELETE FROM %s", table_name);
	res = sqlite_exec(db, sql);
	return res;
}

/*
 * @brief: 清除获取到的数据
 * @param[in]: data: 要消除的数据
 * @return: 返回 EXIT_SUCCESS
 */
int free_data(char** data)
{
	sqlite3_free_table(data);
	return EXIT_SUCCESS;
}
