#pragma once

struct LangOptions {
  enum Standard { C89, C99, C11, C17 } std = C99;
  bool allowLineComments = true;
  bool allowVLA = false;
  bool allowDesignatedInit = false;
  bool allowBool = false;
  bool allowInline = false;
  bool allowRestrict = false;

  static LangOptions forC89() {
    LangOptions o;
    o.std = C89;
    o.allowLineComments = false;
    return o;
  }

  static LangOptions forc99() {
    LangOptions o;
    o.std = C99;
    o.allowLineComments = true;
    o.allowVLA = true;
    o.allowDesignatedInit = true;
    o.allowBool = true;
    o.allowInline = true;
    o.allowRestrict = true;
    return o;
  }
};
