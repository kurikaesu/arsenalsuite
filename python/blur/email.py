
#import email.MIMEText
import smtplib
# Have to load these like this, because this module is
# called email
MIMEText = __import__('email.MIMEText').MIMEText.MIMEText
MIMEMultipart = __import__('email.MIMEMultipart').MIMEMultipart.MIMEMultipart
MIMEBase = __import__('email.MIMEBase').MIMEBase.MIMEBase
Encoders = __import__('email.Encoders').Encoders
import sys
import quopri

def send( sender, recipients, subject, body, attachments=[] ):
	#Ensure python strings
	recipients = [str(r) for r in recipients]
	subject = str(subject)
	body = str(body)
	sender = str(sender)
	
	if not sender:
		return
	
	# Read from config file first
	domain = "@drdstudios.com"
	server = "smtp.drd.roam"
	
	for (i,v) in enumerate(recipients):
		if not v.count('@'):
			if not v.count('@'):
				recipients[i] = v + str(domain)
	
	email = MIMEMultipart()
	email['Subject'] = subject
	email['From'] = sender
	email['To'] = ', '.join(recipients)
	#email['Date'] = str(QDateTime.currentDateTime().toString('ddd, d MMM yyyy hh:mm:ss'))
	email['Content-type']="Multipart/mixed"
	email.preamble = 'This is a multi-part message in MIME format.'
	email.epilogue = ''
	
	#msgAlternative = MIMEMultipart('alternative')
	#email.attach(msgAlternative)
	
	msgText = MIMEText(body)
	msgText['Content-type'] = "text/plain"
	
	#msgAlternative.attach(msgText)
	email.attach(msgText)
	
	for a in attachments:
		fp = open( str(a), 'rb' )
		txt = MIMEBase("application", "octet-stream")
		txt.set_payload(fp.read())
		fp.close()
		Encoders.encode_base64(txt)
		txt.add_header('Content-Disposition', 'attachment; filename="%s"' % a)
		email.attach(txt)
		
	s = smtplib.SMTP()
	mailServer = str(server)
	s.connect(mailServer)
	print email.get_content_maintype()
	print "Sending email to ", recipients
	s.sendmail(sender, recipients, email.as_string())
	s.close()
	return True
