#ifndef __MYFS_H__
#define __MYFS_H__

#include "blkdev.h"
#include <memory>
#include <vector>
#include <stdint.h>


class MyFs {
public:
	MyFs(BlockDeviceSimulator *blkdevsim_);


	/**
	 * dir_list_entry struct
	 * This struct is used by list_dir method to return directory entry
	 * information.
	 */
	struct dir_list_entry {

		/*
		* The inode of the entry.
		*/
		int inode;

		/*
		 * The directory entry name
		 */
		std::string name;

		/*
		 * File size
		 */
		int file_size;

		/*
		 * whether the entry is a file or a directory
		 */
		bool is_dir;
	};
	typedef std::vector<struct dir_list_entry> dir_list;


	struct dir_list_data {
		/*
		* The inode of the entry.
		*/
		int inode;

		/*
		* The contents of the file.
		*/
		std::string contents;
	};
	typedef std::vector<struct dir_list_data> data_list;

	/**
	 * format method
	 * This function discards the current content in the blockdevice and
	 * create a fresh new MYFS instance in the blockdevice.
	 */
	void format();

	/**
	 * create_file method
	 * Creates a new file in the required path.
	 * @param path_str the file path (e.g. "/newfile")
	 * @param directory boolean indicating whether this is a file or directory
	 */
	void create_file(std::string path_str, bool directory);

	/**
	 * get_content method
	 * Returns the whole content of the file indicated by path_str param.
	 * Note: this method assumes path_str refers to a file and not a
	 * directory.
	 * @param path_str the file path (e.g. "/somefile")
	 * @return the content of the file
	 */
	std::string get_content(std::string path_str);

	/**
	 * set_content method
	 * Sets the whole content of the file indicated by path_str param.
	 * Note: this method assumes path_str refers to a file and not a
	 * directory.
	 * @param path_str the file path (e.g. "/somefile")
	 * @param content the file content string
	 */
	void set_content(std::string path_str, std::string content);

	/**
	 * list_dir method
	 * Returns a list of a files in a directory.
	 * Note: this method assumes path_str refers to a directory and not a
	 * file.
	 * @param path_str the file path (e.g. "/somedir")
	 * @return a vector of dir_list_entry structures, one for each file in
	 *	the directory.
	 */
	dir_list list_dir(std::string path_str);

private:

	void Parse(std::string str_parse);
	void parseEntries(std::string toParse);
	void parseContents(std::string toParse);
	void updateContent(int inode, std::string content);
	void updateFileSize(int inode, int newSize);
	void updateBlockDev(bool type, std::string toInsert); //If type true - insert entry, false - insert contents.


	/**
	 * This struct represents the first bytes of a myfs filesystem.
	 * It holds some magic characters and a number indicating the version.
	 * Upon class construction, the magic and the header are tested - if
	 * they both exist than the file is assumed to contain a valid myfs
	 * instance. Otherwise, the blockdevice is formated and a new instance is
	 * created.
	 */
	struct myfs_header {
		char magic[4];
		uint8_t version;
	};

	BlockDeviceSimulator *blkdevsim;

	static const uint8_t CURR_VERSION = 0x03;
	static const char *MYFS_MAGIC;

	dir_list EntriesVec;
	data_list ContentsVec;
	std::string BlockDevStrData;
	int last_inode = 0;
	int headerSize;

};

#endif // __MYFS_H__
