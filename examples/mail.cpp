#include <axon.h>
#include <axon/mail.h>

int main()
{
	axon::mail mailman;

	mailman[AXON_MAIL_SERVER] = "appmail1.domain.com";
	mailman[AXON_MAIL_USERNAME] = "username";
	mailman[AXON_MAIL_PASSWORD] = "password";
	mailman[AXON_MAIL_FROM] = "user@domain.com";
	mailman[AXON_MAIL_PORT] = 25;

	try {

		if (mailman.connect())
		{
			mailman.to("someone@domain.com");
			mailman.cc("someother@domain.com");
			mailman.subject("A Test Mail");
			mailman.text("open email for full body");
			//mailman.html("dGhpcyBpcyBhIG5vdCA8aDE+c2ltcGxlPC9oMT4gdGV4dCBib2R5");
			// mailman.html("this is a not <h1>simple</h1> text body");
			mailman.html("/opt/hyperion/assets/email.html", true);
			mailman.attach("extras/logo.png", "logo.png");

			mailman.send();
			mailman.disconnect();
		}
	} catch (axon::exception &e) {

		std::cerr<<"ERROR: "<<e.what()<<std::endl;
	}

	return 0;
}