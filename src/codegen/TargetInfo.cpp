#include "../../include/cc1/TargetInfo.hpp"

TargetInfo i386() {
  TargetInfo T;
  T.arch = TargetInfo::I386;
  T.sizeofLong = 4;
  T.sizeofPointer = 4;
  T.maxAlign = 4;
  T.datalayout = "e-m:e-p:32:32-i64:64-f80:32-n8:16:32-S32";
  T.triple = "i386-pc-linux-gnu";
  return T;
}

TargetInfo x86_64() {
  TargetInfo T;
  T.arch = TargetInfo::X86_64;
  T.sizeofLong = 8;
  T.sizeofPointer = 8;
  T.maxAlign = 16;
  T.datalayout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128";
  T.triple = "x86_64-pc-linux-gnu";
  return T;
}

TargetInfo aarch64() {
  TargetInfo T;
  T.arch = TargetInfo::AARCH64;
  T.sizeofLong = 8;
  T.sizeofPointer = 8;
  T.maxAlign = 16;
  T.datalayout = "e-m:o-i64:64-i128:128-n32:64-S128";
  T.triple = "aarch64-apple-macosx13.0.0";
  return T;
}
