
#include "plugin.h"


DatePickerPlugin::DatePickerPlugin(QObject *parent)
    : QObject(parent),
      m_initialized(false)
{
}

DatePickerPlugin::~DatePickerPlugin()
{
}

QString DatePickerPlugin::name() const
{
    return QLatin1String("KDatePicker");
}

QString DatePickerPlugin::group() const
{
    return QLatin1String("Stone Widgets");
}

QString DatePickerPlugin::toolTip() const
{
    return QString();
}

QString DatePickerPlugin::whatsThis() const
{
    return QString();
}

QString DatePickerPlugin::includeFile() const
{
    return QLatin1String("kdatepicker.h");
}

QIcon DatePickerPlugin::icon() const
{
    return QIcon();
}

bool DatePickerPlugin::isContainer() const
{
    return false;
}

QWidget *DatePickerPlugin::createWidget(QWidget *parent)
{
	return new KDatePicker( parent );
}

QString DatePickerPlugin::domXml() const
{
	 return QLatin1String("\
	<widget class=\"DatePicker\" name=\"mLineEdit\">\
		<property name=\"geometry\">\
			<rect>\
				<x>0</x>\
				<y>0</y>\
				<width>100</width>\
				<height>30</height>\
			</rect>\
		</property>\
	</widget>\
	");
}

Q_EXPORT_PLUGIN( DatePickerPlugin )
