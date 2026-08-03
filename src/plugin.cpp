#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    p->addModel(modelClairaudient);
    p->addModel(modelChiaroscuro);
    p->addModel(modelInvolution);
    p->addModel(modelSpecula);
    p->addModel(modelUtilityPanel);
}
