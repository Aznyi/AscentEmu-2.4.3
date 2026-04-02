#ifndef SVN_REVISION_H
#define SVN_REVISION_H

#include "git_revision.h"

#ifndef BUILD_SOURCE_TEXT
# define BUILD_SOURCE_TEXT "git"
#endif

#ifndef BUILD_BRANCH_TEXT
# define BUILD_BRANCH_TEXT "unknown"
#endif

#ifndef BUILD_TAG_TEXT
# define BUILD_TAG_TEXT BUILD_BRANCH_TEXT
#endif

#ifndef BUILD_HASH_TEXT
# define BUILD_HASH_TEXT "unknown"
#endif

#ifndef BUILD_REVISION_NUMBER
# define BUILD_REVISION_NUMBER 0
#endif

static const char * BUILD_SOURCE = BUILD_SOURCE_TEXT;
static const char * BUILD_BRANCH = BUILD_BRANCH_TEXT;
static const char * BUILD_TAG = BUILD_TAG_TEXT;
static const char * BUILD_HASH = BUILD_HASH_TEXT;
static int BUILD_REVISION = BUILD_REVISION_NUMBER;

#endif         // SVN_REVISION_H
