/**
 *  @file
 *  @author     2012-2014 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 *  @endcond
 */

#include "ChartToFlatSCXML.h"
#include "uscxml/transform/FlatStateIdentifier.h"

#define CREATE_TRANSIENT_STATE_WITH_CHILDS(stateId) \
if (childs.size() > 0) { \
	Element<std::string> transientState = _flatDoc.createElementNS(_nsInfo.nsURL, "state"); \
	_nsInfo.setPrefix(transientState);\
	transientState.setAttribute("transient", "true"); \
	if (stateId.length() > 0) \
		transientState.setAttribute("id", stateId); \
	for (int i = 0; i < childs.size(); i++) { \
		Node<std::string> imported = _flatDoc.importNode(childs[i], true); \
		transientState.appendChild(imported); \
	} \
	transientStateChain.push_back(transientState); \
} \
childs = NodeSet<std::string>();

namespace uscxml {

using namespace Arabica::DOM;
using namespace Arabica::XPath;

ChartToFlatSCXML::operator Interpreter() {
	if (!HAS_ATTR(_scxml, "flat") || !DOMUtils::attributeIsTrue(ATTR(_scxml, "flat"))) {
		createDocument();
	}
	return Interpreter::fromClone(shared_from_this());
}
	
Transformer ChartToFlatSCXML::transform(const Interpreter& other) {
	return boost::shared_ptr<TransformerImpl>(new ChartToFlatSCXML(other));
}

void ChartToFlatSCXML::writeTo(std::ostream& stream) {
	if (!HAS_ATTR(_scxml, "flat") || !DOMUtils::attributeIsTrue(ATTR(_scxml, "flat"))) {
		createDocument();
	}
	stream << _scxml;
}

void ChartToFlatSCXML::createDocument() {

	if (HAS_ATTR(_scxml, "flat") && DOMUtils::attributeIsTrue(ATTR(_scxml, "flat")))
		return;
	
	if (_start == NULL)
		interpret(); // only if not already flat!
	
	Element<std::string> _origSCXML = _scxml;
	
	_scxml = _flatDoc.createElementNS(_nsInfo.nsURL, "scxml");
	_nsInfo.setPrefix(_scxml);
	
	_scxml.setAttribute("flat", "true");
	_flatDoc.appendChild(_scxml);
	
	if (HAS_ATTR(_origSCXML, "datamodel")) {
		_scxml.setAttribute("datamodel", ATTR(_origSCXML, "datamodel"));
	}
	
	if (HAS_ATTR(_origSCXML, "name")) {
		_scxml.setAttribute("name", ATTR(_origSCXML, "name"));
	}
	
	if (HAS_ATTR(_origSCXML, "binding")) {
		_scxml.setAttribute("binding", ATTR(_origSCXML, "binding"));
	}
	
	_scxml.setAttribute("initial", _start->stateId);
	
	NodeSet<std::string> datas;
	if (_binding == InterpreterImpl::LATE) {
		// with late binding, just copy direct datamodel childs
		datas = filterChildElements(_nsInfo.xmlNSPrefix + "datamodel", _origSCXML);
	} else {
		// with early binding, copy all datamodel elements into scxml element
		datas = _xpath.evaluate("//" + _nsInfo.xpathPrefix + "datamodel", _origSCXML).asNodeSet();
	}
	for (int i = 0; i < datas.size(); i++) {
		if (isInEmbeddedDocument(datas[i]))
			continue; // nested document
		Node<std::string> imported = _flatDoc.importNode(datas[i], true);
		_scxml.appendChild(imported);
	}
	
	
	NodeSet<std::string> scripts = filterChildElements(_nsInfo.xmlNSPrefix + "script", _origSCXML);
	for (int i = 0; i < scripts.size(); i++) {
		Node<std::string> imported = _flatDoc.importNode(scripts[i], true);
		_scxml.appendChild(imported);
	}
	
	NodeSet<std::string> comments = filterChildType(Node_base::COMMENT_NODE, _origSCXML);
	for (int i = 0; i < comments.size(); i++) {
		Node<std::string> imported = _flatDoc.importNode(comments[i], true);
		_scxml.appendChild(imported);
	}
	
	std::vector<std::pair<std::string,GlobalState*> > sortedStates;
	sortedStates.insert(sortedStates.begin(), _globalConf.begin(), _globalConf.end());
	std::sort(sortedStates.begin(), sortedStates.end(), sortStatesByIndex);
	
	//	int index = 0;
	//	for (std::vector<Element<std::string> >::iterator transIter = indexedTransitions.begin(); transIter != indexedTransitions.end(); transIter++) {
	//		const Element<std::string>& refTrans = *transIter;
	//		std::cerr << index++ << ": " << refTrans << std::endl;
	//	}
	//	std::cerr << std::endl;
	
	for (std::vector<std::pair<std::string,GlobalState*> >::iterator confIter = sortedStates.begin();
			 confIter != sortedStates.end();
			 confIter++) {
		appendGlobalStateNode(confIter->second);
	}

	_document = _flatDoc;
}

void ChartToFlatSCXML::appendGlobalStateNode(GlobalState* globalState) {
	Element<std::string> state = _flatDoc.createElementNS(_nsInfo.nsURL, "state");
	_nsInfo.setPrefix(state);
	
	state.setAttribute("ref", toStr(globalState->index));
	state.setAttribute("id", globalState->stateId);
	
	if (globalState->isFinal)
		state.setAttribute("final", "true");
	
	std::list<GlobalTransition*>& transitionList = globalState->sortedOutgoing;
	
	// apend here, for transient state chains to trail the state
	_scxml.appendChild(state);
	
	size_t index = 0;
	for (std::list<GlobalTransition*>::iterator outIter = transitionList.begin();
			 outIter != transitionList.end();
			 outIter++) {
//		(*outIter)->index = globalState->index + ":" + toStr(index);
		state.appendChild(globalTransitionToNode(*outIter));
		index++;
	}
}

/**
 * Creates transient states for executable content as a side-effect
 */
Node<std::string> ChartToFlatSCXML::globalTransitionToNode(GlobalTransition* globalTransition) {
	Element<std::string> transition = _flatDoc.createElementNS(_nsInfo.nsURL, "transition");
	_nsInfo.setPrefix(transition);
	
	//	transition.setAttribute("ref", globalTransition->index);
	
#if 1
	transition.setAttribute("members", globalTransition->members);
#endif
	//	transition.setAttribute("priority", toStr(globalTransition->priority));
	
	if (!globalTransition->isEventless) {
		transition.setAttribute("event", globalTransition->eventDesc);
	}
	
	if (globalTransition->condition.size() > 0) {
		transition.setAttribute("cond", globalTransition->condition);
	}
	
	if (globalTransition->destination.size() > 0) {
		transition.setAttribute("final-target", globalTransition->destination);
	}
	
	NodeSet<std::string> transientStateChain;
	
	// current active state set
	FlatStateIdentifier flatId(globalTransition->source);
	std::list<std::string> currActiveStates = flatId.getActive();
	
	//	std::cerr << "From " << globalTransition->source << " to " << globalTransition->destination << ":" << std::endl;
	
	// gather content for new transient state
	NodeSet<std::string> childs;
	// iterate all actions taken during the transition
	for (std::list<GlobalTransition::Action>::iterator actionIter = globalTransition->actions.begin();
			 actionIter != globalTransition->actions.end();
			 actionIter++) {
		
		if (actionIter->transition) {
			//			DETAIL_EXEC_CONTENT(transition, actionIter);
			
			Element<std::string> onexit = _flatDoc.createElementNS(_nsInfo.nsURL, "onexit");
			_nsInfo.setPrefix(onexit);
			Node<std::string> child = actionIter->transition.getFirstChild();
			while(child) {
				Node<std::string> imported = _flatDoc.importNode(child, true);
				onexit.appendChild(imported);
				child = child.getNextSibling();
			}
			// only append if there is something done
			if (onexit.hasChildNodes())
				childs.push_back(onexit);
			
			continue;
		}
		
		if (actionIter->onExit) {
			//			DETAIL_EXEC_CONTENT(onExit, actionIter);
			
			childs.push_back(actionIter->onExit);
			continue;
		}
		
		if (actionIter->onEntry) {
			//			DETAIL_EXEC_CONTENT(onEntry, actionIter);
			
			childs.push_back(actionIter->onEntry);
			continue;
		}
		
		if (actionIter->invoke) {
			//			DETAIL_EXEC_CONTENT(invoke, actionIter);
			if (!globalTransition->isTargetless) {
				//				CREATE_TRANSIENT_STATE_WITH_CHILDS(FlatStateIdentifier::toStateId(currActiveStates));
			}
			Element<std::string> invokeElem = Element<std::string>(actionIter->invoke);
			invokeElem.setAttribute("persist", "true");
			childs.push_back(invokeElem);
			continue;
		}
		
		if (actionIter->uninvoke) {
			//			DETAIL_EXEC_CONTENT(uninvoke, actionIter);
			Element<std::string> uninvokeElem = _flatDoc.createElementNS(_nsInfo.nsURL, "uninvoke");
			_nsInfo.setPrefix(uninvokeElem);
			
			if (HAS_ATTR(actionIter->uninvoke, "type")) {
				uninvokeElem.setAttribute("type", ATTR(actionIter->uninvoke, "type"));
			}
			if (HAS_ATTR(actionIter->uninvoke, "typeexpr")) {
				uninvokeElem.setAttribute("typeexpr", ATTR(actionIter->uninvoke, "typeexpr"));
			}
			if (HAS_ATTR(actionIter->uninvoke, "id")) {
				uninvokeElem.setAttribute("id", ATTR(actionIter->uninvoke, "id"));
			}
			if (HAS_ATTR(actionIter->uninvoke, "idlocation")) {
				uninvokeElem.setAttribute("idlocation", ATTR(actionIter->uninvoke, "idlocation"));
			}
			childs.push_back(uninvokeElem);
			continue;
		}
		
		if (actionIter->exited) {
			//			std::cerr << " exited(" << ATTR_CAST(actionIter->exited, "id") << ")";
			currActiveStates.remove(ATTR_CAST(actionIter->exited, "id"));
			if (childs.size() > 0) {
				CREATE_TRANSIENT_STATE_WITH_CHILDS(FlatStateIdentifier::toStateId(currActiveStates)); // create a new transient state to update its id
			}
		}
		
		if (actionIter->entered) {
			//			std::cerr << " entered(" << ATTR_CAST(actionIter->entered, "id") << ")";
			if (childs.size() > 0)
				CREATE_TRANSIENT_STATE_WITH_CHILDS(FlatStateIdentifier::toStateId(currActiveStates)); // create a new transient state to update its id
			currActiveStates.push_back(ATTR_CAST(actionIter->entered, "id"));
			
			// we entered a new child - check if it has a datamodel and we entered for the first time
			if (_binding == InterpreterImpl::LATE) {
				NodeSet<std::string> datamodel = filterChildElements(_nsInfo.xmlNSPrefix + "datamodel", actionIter->entered);
				if (datamodel.size() > 0 && !isMember(actionIter->entered, _globalConf[globalTransition->source]->getAlreadyEnteredStates())) {
					childs.push_back(datamodel);
				}
			}
		}
		if (!globalTransition->isTargetless) {
			//			CREATE_TRANSIENT_STATE_WITH_CHILDS(FlatStateIdentifier::toStateId(currActiveStates))
		}
	}
	
	//	std::cerr << std::endl;
	
	//	if (globalTransition->isTargetless) {
	//		for (int i = 0; i < childs.size(); i++) {
	//			Node<std::string> imported = _flatDoc.importNode(childs[i], true);
	//			transition.appendChild(imported);
	//			//			CREATE_TRANSIENT_STATE_WITH_CHILDS(FlatStateIdentifier::toStateId(currActiveStates))
	//		}
	//		return transition;
	//	}
	
	CREATE_TRANSIENT_STATE_WITH_CHILDS(FlatStateIdentifier::toStateId(currActiveStates))
	
	if (transientStateChain.size() > 0) {
		Element<std::string> prevExitTransitionElem;
		
		for (int i = 0; i < transientStateChain.size(); i++) {
			Element<std::string> transientStateElem = Element<std::string>(transientStateChain[i]);
			transientStateElem.setAttribute("id", transientStateElem.getAttribute("id") + "-via-" + toStr(_lastTransientStateId++));
			
			Element<std::string> exitTransition = _flatDoc.createElementNS(_nsInfo.nsURL, "transition");
			_nsInfo.setPrefix(exitTransition);
			
			if (prevExitTransitionElem) {
				// point previous to this one
				prevExitTransitionElem.setAttribute("target", transientStateElem.getAttribute("id"));
			} else {
				// update globalTransition->source target
			}
			
			transientStateElem.appendChild(exitTransition);
			prevExitTransitionElem = exitTransition;
			
			if (i == 0)
				transition.setAttribute("target", transientStateElem.getAttribute("id"));
			
			_scxml.appendChild(transientStateElem);
		}
		
		// last one points to actual target
		assert(prevExitTransitionElem);
		prevExitTransitionElem.setAttribute("target", globalTransition->destination);
		
	} else {
		transition.setAttribute("target", globalTransition->destination);
	}
	
	assert(HAS_ATTR_CAST(transition, "target"));
	return transition;
}

bool ChartToFlatSCXML::sortStatesByIndex(const std::pair<std::string,GlobalState*>& s1, const std::pair<std::string,GlobalState*>& s2) {
	return s1.second->index < s2.second->index;
}

}