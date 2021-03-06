#include "uscxml/config.h"
#include "uscxml/Interpreter.h"
#include <glog/logging.h>

using namespace uscxml;

std::set<std::string> issueLocationsForXML(const std::string xml) {
	Interpreter interpreter = Interpreter::fromXML(xml);
	std::list<InterpreterIssue> issues = interpreter.validate();

	std::set<std::string> issueLocations;

	for (std::list<InterpreterIssue>::iterator issueIter = issues.begin(); issueIter != issues.end(); issueIter++) {
		std::cout << *issueIter << std::endl;
		issueLocations.insert(issueIter->xPath);
	}
	return issueLocations;
}

size_t runtimeIssues;
class IssueMonitor : public InterpreterMonitor {
public:
	IssueMonitor() {
		runtimeIssues = 0;
	}
	void reportIssue(Interpreter interpreter, const InterpreterIssue& issue) {
		runtimeIssues++;
	}
};

int main(int argc, char** argv) {

	google::InitGoogleLogging(argv[0]);
	google::LogToStderr();

	int iterations = 1;

	while(iterations--) {

		if (1) {
			// Potential endless loop

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<datamodel><data id=\"counter\" expr=\"5\" /></datamodel>"
			    "	<state id=\"foo\">"
			    "		<onentry><script>counter--;</script></onentry>"
			    "		<transition target=\"foo\" cond=\"counter > 0\" />"
			    "		<transition target=\"bar\" cond=\"counter == 0\" />"
			    " </state>"
			    "	<state id=\"bar\" final=\"true\" />"
			    "</scxml>";

			IssueMonitor monitor;
			Interpreter interpreter = Interpreter::fromXML(xml);
			interpreter.addMonitor(&monitor);
			interpreter.interpret();

			// first reiteration is not counted as it might be valid when raising internal errors
			assert(runtimeIssues == 3);
		}

		if (1) {
			// Unreachable states

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"foo\">"
			    "		<parallel id=\"foz\">"
			    "			<state id=\"s0\" />"
			    "			<state id=\"s1\" />"
			    "			<state id=\"s2\" />"
			    "		</parallel>"
			    " </state>"
			    "	<state id=\"bar\" />"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"bar\"]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Invalid parents

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "		<onentry>"
			    "			<cancel sendidexpr=\"foo\" />"
			    "		</onentry>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("/scxml[1]/onentry[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// State has no 'id' attribute
			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state>"
			    "		<transition/>"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("/scxml[1]/state[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Duplicate state with id
			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\" />"
			    " <state id=\"start\" />"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Transition has non-existant target state

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<transition target=\"done\" />"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/transition[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}


		if (1) {
			// Transition can never be optimally enabled (conditionless, eventless)

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<transition target=\"done\" />"
			    "		<transition target=\"done\" />"
			    " </state>"
			    " <final id=\"done\" />"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/transition[2]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Transition can never be optimally enabled (conditionless, more events)

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<transition event=\"error\" target=\"done\" />"
			    "		<transition event=\"error.bar error.foo\" />"
			    " </state>"
			    " <final id=\"done\" />"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/transition[2]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}


		if (1) {
			// Initial attribute has invalid target state

			const char* xml =
			    "<scxml datamodel=\"ecmascript\" initial=\"foo\">"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("/scxml[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Invoke with unknown type

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<invoke type=\"non-existant\" />"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/invoke[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Send to unknown IO Processor

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<onentry>"
			    "			<send type=\"non-existant\" />"
			    "		</onentry>"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/send[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// SCXML document requires unknown datamodel

			const char* xml =
			    "<scxml datamodel=\"non-existant\">"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("/scxml[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Unknown executable content element

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<onentry>"
			    "			<nonexistant />"
			    "		</onentry>"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/nonexistant[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Syntax error in script

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<script>"
			    "   $wfwegr^ "
			    " </script>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("/scxml[1]/script[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Syntax error in cond attribute

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<onentry>"
			    "			<if cond=\"%2345\">"
			    "			<elseif cond=\"%2345\" />"
			    "			</if>"
			    "		</onentry>"
			    "		<transition cond=\"%2345\" />"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/transition[1]") != issueLocations.end());
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/if[1]") != issueLocations.end());
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/if[1]/elseif[1]") != issueLocations.end());
			assert(issueLocations.size() == 3);
		}

		if (1) {
			// Syntax error in expr attribute

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<datamodel>"
			    "		<data id=\"foo\" expr=\"%2345\" />"
			    "	</datamodel>"
			    "	<state id=\"start\">"
			    "		<onentry>"
			    "			<log expr=\"%2345\" />"
			    "			<assign location=\"foo\" expr=\"%2345\" />"
			    "			<send>"
			    "				<param expr=\"%2345\" />"
			    "				<content expr=\"%2345\" />"
			    "			</send>"
			    "		</onentry>"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/log[1]") != issueLocations.end());
			assert(issueLocations.find("//data[@id=\"foo\"]") != issueLocations.end());
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/assign[1]") != issueLocations.end());
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/send[1]/content[1]") != issueLocations.end());
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/send[1]/param[1]") != issueLocations.end());
			assert(issueLocations.size() == 5);
		}

		if (1) {
			// Syntax error with foreach

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<onentry>"
			    "			<foreach item=\"%2345\" index=\"%2345\" array=\"%2345\">"
			    "			</foreach>"
			    "		</onentry>"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/foreach[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Syntax error with send

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<onentry>"
			    "			<send eventexpr=\"%2345\" targetexpr=\"%2345\" typeexpr=\"%2345\" idlocation=\"%2345\" delayexpr=\"%2345\" />"
			    "		</onentry>"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/send[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Syntax error with invoke

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<invoke typeexpr=\"%2345\" srcexpr=\"%2345\" idlocation=\"%2345\" />"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/invoke[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}

		if (1) {
			// Syntax error with cancel

			const char* xml =
			    "<scxml datamodel=\"ecmascript\">"
			    "	<state id=\"start\">"
			    "		<onentry>"
			    "			<cancel sendidexpr=\"%2345\" />"
			    "		</onentry>"
			    " </state>"
			    "</scxml>";

			std::set<std::string> issueLocations = issueLocationsForXML(xml);
			assert(issueLocations.find("//state[@id=\"start\"]/onentry[1]/cancel[1]") != issueLocations.end());
			assert(issueLocations.size() == 1);
		}


	}

	return EXIT_SUCCESS;
}