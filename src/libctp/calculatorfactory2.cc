#include <votca/ctp/calculatorfactory2.h>
#include "votca_config.h"

#include "calculators/sandbox2.h"
#include "calculators/neighborlist2.h"
#include "calculators/stateserver.h"

namespace votca { namespace ctp {

void CalculatorFactory2::RegisterAll(void)
{	
        Calculators().Register<Sandbox2>("sandbox2"); // Test calculator
        Calculators().Register<Neighborlist2>("neighborlist2");
        Calculators().Register<StateServer>("stateserver");
}

}}
