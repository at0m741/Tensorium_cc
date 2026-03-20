#pragma once
#include <string>

struct TargetInfo {
  enum Arch { I386, X86_64, AARCH64 } arch;

  int sizeofShort = 2;
  int sizeofInt = 4;
  int sizeofLong;
  int sizeofLonglong = 8;

  int sizeofPointer;
  int sizeofFloat = 4;
  int sizeofDouble = 8;
  int maxAlign;

  std::string datalayout;
  std::string triple;

  static TargetInfo i386();
  static TargetInfo x86_64();
  static TargetInfo aarch64();

  int sizeofType(/* Type::Kind */) const;
};
