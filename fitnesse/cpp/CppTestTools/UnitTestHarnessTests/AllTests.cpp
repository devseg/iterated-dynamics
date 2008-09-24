//
// Copyright (c) 2004 Michael Feathers and James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#include "Platform.h"
#include "UnitTestHarness/CommandLineTestRunner.h"
#include "UnitTestHarness/UnitTestHarnessTests.h"
#include "Helpers/HelpersTests.h"

int main(int ac, char** av)
{
  CommandLineTestRunner::RunAllTests(ac, av);
  return 0;
}
