#ifndef _SQLITE_OP_H_
#define _SQLITE_OP_H_

#include "sqlite3.h"

#define SQLITE_PATH_MAX_LEN 128

#define DB_OPEN_TARGET 53

typedef struct sqlite_db
{
	char file_name[SQLITE_PATH_MAX_LEN];
	sqlite3* database;

	int open_target;
}sqlite_db;

int check_sqlite_exist(char* file_name);

int open_sqlite(sqlite_db* db, char* file_name);
int close_sqlite(sqlite_db* db);

/*
 * @brief: 用于执行sql语句，通常是用于增、删、改
 * @param[in]: db: 经过初始化的sqlite_db
 * @param[in]: sql: 要执行的sql语句
 * @return: EXIT_FAILURE(0)代表成功，否则为失败
 */
int sqlite_exec(sqlite_db* db, char* sql);

/*
 * 数据库里面是一个个的表
 * 表有字段+数据
 * 比如 表 score_group
 * id name score
 * 1  zhangsan 94
 * 2  lisi     82
 *
 * 假设 sql 语句是 SELECT * FROM score_group
 * 也就是返回全部数据
 * sqlite_get_data 返回的数据第一行是 id name score 之后才是每条数据
 * sqlite_get_data_out_title 返回的数据第一行就是第一条数据
 * 也就是两者的差距就是缺少了是否返回字段
 * 但是两者返回的 row 和 col都是一样的
 * 例子来说，就是 row = 2 col = 3 是不算入字段的
 *
 * 使用建议
 * char** data = NULL;
 * int row;
 * int col;
 * sqlite_get_data(db, sql, &data, &row, &col);
 * // 使用 data
 *
 * sqlite_get_data_no_err 是 sqlite_get_data 的改版函数
 * sqlite_get_data 和 sqlite_get_data_no_err 结束时这样释放内存
 * free_data(data);
 *
 * sqlite_get_data_out_title 结束时这样释放内存 切记不能用 free_data
 * free_two_dimensional_array((void***)&data, len); //在 tool.h 头文件内
 */
int sqlite_get_data(sqlite_db* db, char* sql, char*** data, int* data_row, int* data_col);
int sqlite_get_data_no_err(sqlite_db* db, char* sql, char*** data, int* data_row, int* data_col);
int sqlite_get_data_out_title(sqlite_db* db, char* sql, char*** data, int* data_row, int* data_col);

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
int sqlite_get_last_id(sqlite_db* db, char* table_name, char** id);
/*
 * @brief: 清除单个表格的内容
 * @param[in]: 参数1(db): 操作的数据库信息数据结构指针
 * @param[in]: 参数2(table_name): 操作要删除的表名字
 * @return: 成功消除 返回 EXIT_SUCCESS 否则 返回 EXIT_FAILURE
 */
int truncate_table(sqlite_db* db, char* table_name);
/*
 * @brief: 清除获取到的数据
 * @param[in]: data: 要消除的数据
 * @return: 返回 EXIT_SUCCESS
 */
int free_data(char** data);

#endif
