# if pluginType is CW_FILTER, the plugin loader will call the getFilter
# method, which must be defined
pluginType = CW_FILTER

# the variables CW_FILTER, MATCH and NO_MATCH are inserted into the
# local namespace by the plugin loader

# getFilter must return a callable that accepts three arguments.
# The filter will be called with widget name, base class name and
# module name as given in the UI file
# the filter must return a tuple
# (match_result, data)
# If the filter matches, "match_result" is MATCH and "data"
# contains the modified argument tuple
# In the other case, "match_result" is NO_MATCH, and data is ignored

# Any other result will load to an error and a program exit
def getFilter():
    def _qwtfilter(widgetname, baseclassname, module):
        if widgetname.startswith("Qwt"):
            return (MATCH, (widgetname, baseclassname, "PyQt4.Qwt5"))
        else:
            return (NO_MATCH, None)

    return _qwtfilter
