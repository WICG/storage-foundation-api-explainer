#include <iostream>
#include <fstream>
#include <cstdio>

#include <emscripten.h>

using namespace std;

extern "C" {
// Executes one step of the Fibonacci sequence, adding the new number to the end
// of the file at path
void step(const char* path) {
  fstream file;
  file.open(path, ios::out | ios::in | ios::binary);
  file.exceptions ( ifstream::failbit | ifstream::badbit );

  unsigned int first, second, result;

  file.seekg(-2 * ((int) sizeof(first)), ios::end);
  file.read((char*) &first, sizeof(first));
  file.seekg(-((int) sizeof(second)), ios::end);
  file.read((char*) &second, sizeof(second));

  result = first + second;

  file.seekp(0, ios::end);
  file.write((char*) &result, sizeof(result));
  file.close();

  EM_ASM({console.log("Added a new Fibonacci number to file:", $0, "+",  $1,"=", $2)}, first, second, result);
}

// Ensures that the files exists and contains enough entries to do a fibonacci
// step
void init(const char* path) {
  fstream file;
  file.exceptions ( ifstream::failbit | ifstream::badbit );
  file.open(path, ios::out | ios::binary);
  streampos begin = file.tellg();
  file.seekg(0, ios::end);
  streampos end = file.tellg();
  if(end-begin < 2) {
    file.close();
    //TODO: use ios::trunc once we support setLength in NativeIO
    remove(path);
    file.open(path, ios::out | ios::binary);
    unsigned int val = 1;
    file.seekp(0, ios::beg);
    file.write((char*) &val, sizeof(val));
    file.seekp(sizeof(val), ios::beg);
    file.write((char*) &val, sizeof(val));
  }
  file.close();
}

}
