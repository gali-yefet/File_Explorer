#include "myfs.h"
#include <string.h>
#include <iostream>
#include <math.h>
#include <sstream>

const char *MyFs::MYFS_MAGIC = "MYFS";

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_):blkdevsim(blkdevsim_) {
	struct myfs_header header;
	blkdevsim->read(0, sizeof(header), (char *)&header);

	if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
	    (header.version != CURR_VERSION)) {
		std::cout << "Did not find myfs instance on blkdev" << std::endl;
		std::cout << "Creating..." << std::endl;
		format();
		std::cout << "Finished!" << std::endl;
	}
	else
	{
		//file exists, update savers
		this->headerSize = sizeof(header);
		int bufferSize = BlockDeviceSimulator::DEVICE_SIZE - sizeof(header);
		char readOutput[bufferSize];

		std::cout << "Reading Data from Block Device..." << std::endl;
		blkdevsim->read(0, bufferSize, readOutput);
		this->BlockDevStrData = std::string(readOutput);
		Parse(this->BlockDevStrData.substr(this->headerSize)); // Put the data into the class.

		std::cout << "Successfully Retrieved the data from the Block Device!" << std::endl;

		
	}
}

void MyFs::format() {

	// put the header in place
	struct myfs_header header;
	strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
	header.version = CURR_VERSION;
	blkdevsim->write(0, sizeof(header), (const char*)&header);

	// TODO: put your format code here

	/*
	(Header)
	{
		[ // Entries
			<inode, name, size, isdir>
			<>
			<>
			...
		]
		#
		[ // Data
			<inode, contents>
			<>
			<>
			...
		]
	}
	*/
	this->blkdevsim->write(sizeof(header) + 1, 8, "{[]#[]}");
	this->BlockDevStrData = std::string((const char*)&header) + "{[]#[]}";

}

void MyFs::create_file(std::string path_str, bool directory) {
	
	if(directory)
		throw std::runtime_error("Did not implement directories!");

	for (size_t i = 0; i < this->EntriesVec.size(); i++)
	{
		if(path_str == this->EntriesVec[i].name)
		{
			throw std::runtime_error("File already exists!");
		}		
	}
	

	// can't set content that contain: # [ ] < > { } , -because it might harm the program
	if (path_str.find("[") != std::string::npos || path_str.find("]") != std::string::npos || 
		path_str.find("<") != std::string::npos || path_str.find(">") != std::string::npos || 
		path_str.find("{") != std::string::npos || path_str.find("}") != std::string::npos || 
		path_str.find("#") != std::string::npos || path_str.find(",") != std::string::npos)
		throw std::runtime_error("Invalid Character(s) given! (Can't use these: # [ ] < > { } ,)\n");
	
	dir_list_entry entryStruct = {last_inode + 1, path_str, 0, directory};
	this->EntriesVec.push_back(entryStruct);

	std::string update = "<";
	update += (std::to_string(last_inode + 1) + ",");
	update += (path_str + ",");
	update += "0,";
	if(directory)
		update += "true>";
	else
		update += "false>";

	updateBlockDev(true, update);
	this->last_inode++;

}

std::string MyFs::get_content(std::string path_str) {

	for (size_t i = 0; i < this->EntriesVec.size(); i++)
	{
		if(this->EntriesVec[i].name == path_str)
		{
			for (size_t j = 0; j < this->ContentsVec.size(); j++)
			{
				if(this->EntriesVec[i].inode == this->ContentsVec[j].inode)
					return this->ContentsVec[j].contents;
			}
		}
	}
	throw std::runtime_error(std::string("File is empty or does not exist!\n"));

}

void MyFs::set_content(std::string path_str, std::string content) {

	// can't set content that contain: # [ ] < > { } , -because it might harm the program
	if (content.find("[") != std::string::npos || content.find("]") != std::string::npos || 
		content.find("<") != std::string::npos || content.find(">") != std::string::npos || 
		content.find("{") != std::string::npos || content.find("}") != std::string::npos || 
		content.find("#") != std::string::npos || content.find(",") != std::string::npos)
		throw std::runtime_error("Invalid Character(s) given! (Can't use these: # [ ] < > { } ,)\n");

	for (size_t i = 0; i < this->EntriesVec.size(); i++)
	{
		if(this->EntriesVec[i].name == path_str)
		{
			for (size_t j = 0; j < this->ContentsVec.size(); j++)
			{
				if(this->EntriesVec[i].inode == this->ContentsVec[j].inode)
				{
					updateContent(this->EntriesVec[i].inode, content);
					updateFileSize(this->EntriesVec[i].inode, content.size());
					this->EntriesVec[i].file_size = content.size();
					this->ContentsVec[j].contents = content;
					return;
				}
			}
			std::string update = "<";
			update += std::to_string(this->EntriesVec[i].inode) + ",";
			update += content + ">";
			updateBlockDev(false, update);
			dir_list_data structToInsert = {this->EntriesVec[i].inode, content};
			this->ContentsVec.push_back(structToInsert);
			updateFileSize(this->EntriesVec[i].inode, content.size());
			this->EntriesVec[i].file_size = content.size();
			return;
		}
	}
	throw std::runtime_error(std::string("File does not exist!\n"));

}


MyFs::dir_list MyFs::list_dir(std::string path_str) {//Didn't use path, not compatible with directories.

	if(this->EntriesVec.size() == 0)
		throw std::runtime_error(std::string("Empty Directory!\n"));
	else
		return this->EntriesVec;

}

//private functions


void MyFs::Parse(std::string str_parse)
{	
	parseEntries(str_parse);
	parseContents(str_parse);
}

void MyFs::updateBlockDev(bool type, std::string toInsert)
{
	int begin = 0;
	std::string temp = this->BlockDevStrData;

	if(type)
		begin = temp.find("[") + 1;
	else
		begin = temp.find("#") + 2;

	temp.insert(begin, toInsert);
	this->BlockDevStrData = temp;
	this->blkdevsim->write(0, BlockDevStrData.length(), BlockDevStrData.c_str());
}

void MyFs::updateContent(int inode, std::string content)
{
	
	std::string newData;
	
	newData = "<";
	newData += std::to_string(inode) + ",";
	newData += content + ">";

	std::string oldData;

	for (size_t i = 0; i < this->EntriesVec.size(); i++)
	{
		for (size_t j = 0; j < this->ContentsVec.size(); j++)
		{
			if (inode == this->EntriesVec[i].inode && inode == this->ContentsVec[j].inode)
			{
				oldData = "<";
				oldData += std::to_string(this->EntriesVec[i].inode) + ",";
				oldData += this->ContentsVec[i].contents + ">";
			}
		}
	}

	std::string temp = this->BlockDevStrData.substr(0, this->BlockDevStrData.find(oldData));
	temp += newData;
	temp += this->BlockDevStrData.substr(this->BlockDevStrData.find(oldData) + oldData.length());
	this->BlockDevStrData = temp;
	this->blkdevsim->write(0, this->BlockDevStrData.size(), this->BlockDevStrData.c_str());
}

void MyFs::updateFileSize(int inode, int newSize)
{
	std::string newData;
	std::string oldData;
	for (size_t i = 0; i < this->EntriesVec.size(); i++)
	{
		if (inode == this->EntriesVec[i].inode)
		{
			oldData = "<";
			oldData += std::to_string(this->EntriesVec[i].inode) + ",";
			oldData += this->EntriesVec[i].name + ",";
			oldData += std::to_string(this->EntriesVec[i].file_size) + ",";

			newData = "<";
			newData += std::to_string(this->EntriesVec[i].inode) + ",";
			newData += this->EntriesVec[i].name + ",";
			newData += std::to_string(newSize) + ",";

			if(this->EntriesVec[i].is_dir)
			{
				newData += "true>";
				oldData += "true>";
				this->EntriesVec[i] = {inode, this->EntriesVec[i].name, newSize, true};
			}
			else
			{
				newData += "false>";
				oldData += "false>";
				this->EntriesVec[i] = {inode, this->EntriesVec[i].name, newSize, false};
			}
		}
	}
	std::string temp = this->BlockDevStrData.substr(0,  this->BlockDevStrData.find(oldData));
	temp += newData;
	temp += this->BlockDevStrData.substr(this->BlockDevStrData.find(oldData) + oldData.length());
	this->BlockDevStrData = temp;
	this->blkdevsim->write(0, this->BlockDevStrData.size(), this->BlockDevStrData.c_str());
}

void MyFs::parseEntries(std::string toParse)
{
	unsigned int seperatorPos = toParse.find("#");
	unsigned int begin = 2;
	unsigned int prev = 0;
	unsigned int cur = 0;

	while (cur < seperatorPos)
	{
		cur = toParse.find("<", begin);
		
		// If we found an opening < there has to be a closing >.
		prev = cur;
		cur = toParse.find(">", prev);

		if (cur == std::string::npos)
			return;
		else if (cur > seperatorPos)
			return;
		
		std::string sArr[4] = {};
		
		unsigned int len = 0;
		unsigned int prevComma = 0;
		unsigned int curComma = 0;

		curComma = toParse.find(",", prev + 1);
		len = curComma - prev;
		sArr[0] = toParse.substr(prev + 1, len - 1);

		for (size_t i = 1; i < 3; i++)
		{
			prevComma = curComma;
			curComma = toParse.find(",", prevComma + 1);
			len = curComma - prevComma;

			sArr[i] = toParse.substr(prevComma + 1, len - 1);
		}

		prevComma = curComma;
		unsigned int endIndex = toParse.find(">", prevComma + 1);
		len = endIndex - prevComma;

		sArr[3] = toParse.substr(prevComma + 1, len - 1);

		int inode = atoi(sArr[0].c_str());

		if (inode > last_inode)
			last_inode = inode;
		
		std::string name = sArr[1];
		int size = atoi(sArr[2].c_str());
		
		bool isDir = false;
		if(strcmp(sArr[3].c_str(), "false") == 0)
			isDir = false;
		else if(strcmp(sArr[3].c_str(), "true") == 0)
			isDir = true;

		dir_list_entry toPush = (dir_list_entry){inode, name, size, isDir};

		this->EntriesVec.push_back(toPush);

		begin = cur + 1;
	}

}

void MyFs::parseContents(std::string toParse)
{
	unsigned int begin = toParse.find("#") + 1;
	unsigned int cur = 0;
	unsigned int prev;
	unsigned int endPos = toParse.find("}");
	while (cur < endPos)
	{
		cur = toParse.find("<", begin);

		if (cur == std::string::npos) //Not Found.
			return;
		else if (cur >= endPos)
			return;
			
				
		// If we found an opening < there has to be a closing >.
		prev = cur;
		cur = toParse.find(">", prev);
		
		std::string sArr[2] = {};
		unsigned int len = 0;
		unsigned int prevComma;
		unsigned int curComma;

		curComma = toParse.find(",", prev + 1);

		len = curComma - prev;
				
		sArr[0] = toParse.substr(prev + 1, len - 1);

		prevComma = curComma;
		unsigned int curEnd = toParse.find(">", prevComma + 1);
		len = curEnd - prevComma;

		sArr[1] = toParse.substr(prevComma + 1, len - 1);

		int inode = atoi(sArr[0].c_str());
		std::string contents = sArr[1];

		dir_list_data toPush = (struct dir_list_data){inode, contents};
		this->ContentsVec.push_back(toPush);
		begin = cur + 1;
	}
}


