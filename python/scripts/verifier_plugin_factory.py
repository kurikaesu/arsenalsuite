
class VerifierPluginFactory:
    sVerifierFactory = None
    sVerifierPlugins = {}

    def __init__(self):
        pass

    def registerPlugin(self, name, pluginFunc):
        VerifierPluginFactory.sVerifierPlugins[name] = pluginFunc

    def instance():
        if( not VerifierPluginFactory.sVerifierFactory ):
            VerifierPluginFactory.sVerifierFactory = VerifierPluginFactory()
        return VerifierPluginFactory.sVerifierFactory

