
class Submitter : QObject
{
%TypeHeaderCode
#include <submitter.h>
%End
public:
	Submitter( QObject * parent /TransferThis/ = 0 );

	~Submitter();

	enum State {
		New,
		Submitting,
		Success,
		Error
	};

	State state() const;

	void applyArgs( const QMap<QString,QString> & args );

	MappedJob job();
	void setJob( const Job & job );
	void newJobOfType( const JobType & job );
	void setFrameList( const QString & frameList, const QString & taskLabels = QString(), int frameNth = 0, bool frameFill = false );
	void addTasks( JobTaskList jtl );

	void addJobOutput( const QString & outputPath, const QString & outputName = QString(), const QString & frameList = QString(), const QString & taskLabels = QString(), int frameNth = 0, bool frameFill = false );

	void addServices( ServiceList services );
	void addJobDeps( JobDepList deps );

	void addDependencies( JobList deps, uint depType=1 );

	void setExitAppOnFinish( bool );

	bool submitSuspended() const;

	State waitForFinished();

	QString errorText() const;	

signals:
	void submitError( const QString & error );
	void submitSuccess();
	void stateChange( State, const QString & subState );
	void uploadProgress( int );

public slots:
	void submit();
	void setSubmitSuspended( bool );

};
