/* SPDX-License-Identifier: MPL-2.0 */

#include "SmolMachines.h"

DEFINE_LOG_CATEGORY(LogSmolMachinesCpp);

void FSmolMachinesModule::StartupModule()
{
}

void FSmolMachinesModule::ShutdownModule()
{
}
	
IMPLEMENT_MODULE(FSmolMachinesModule, SmolMachines)