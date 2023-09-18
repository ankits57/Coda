#include "../../FFI/FFI.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

extern "C" EXPORT void coda_file_exists(IValuePtr res, IValuePtr args, IEnvironment * env) {
	const std::string filePath = args->getProperties()["filePath"]->getValue();

	// Use std::filesystem to check if the file exists
	if (std::filesystem::exists(filePath)) {
		res->setType(Coda::Runtime::Type::BOOL);
		res->setValue("1"); // File exists
	}
	else {
		res->setType(Coda::Runtime::Type::BOOL);
		res->setValue("0"); // File does not exist
	}
}

extern "C" EXPORT void coda_rename(IValuePtr res, IValuePtr args, IEnvironment * env) {
	const std::string oldPath = args->getProperties()["oldPath"]->getValue();
	const std::string newPath = args->getProperties()["newPath"]->getValue();

	// Use std::filesystem to rename or move the file/directory
	std::filesystem::rename(oldPath, newPath);
	{
		res->setType(Coda::Runtime::Type::BOOL);
		res->setValue("1"); // Rename/move success
	}
}


extern "C" EXPORT void coda_list_files(IValuePtr res, IValuePtr args, IEnvironment * env) {
	const std::string dirPath = args->getProperties()["dirPath"]->getValue();
	std::vector<std::string> fileList;

	// Use std::filesystem to list files and subdirectories
	for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
		fileList.push_back(entry.path().string());
	}

	auto resProps = res->getProperties();
	unsigned int count = 1;
	for (auto item : fileList) {
		resProps.insert({ std::to_string(count),res->createNewValue(Coda::Runtime::Type::STRING,item) });
		count++;
	}

	res->setType(Coda::Runtime::Type::LIST);
	res->setProperties(resProps);
}