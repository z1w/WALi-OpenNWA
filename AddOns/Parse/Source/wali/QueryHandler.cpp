/*!
 * @author Nicholas Kidd
 */

#include "StrX.hpp"

#include "wali/Common.hpp"
#include "wali/Key.hpp"
#include "wali/MergeFnFactory.hpp"
#include "wali/QueryHandler.hpp"
#include "wali/WeightFactory.hpp"

#include "wali/wfa/WFA.hpp"
#include "wali/wfa/WfaHandler.hpp"

#include "wali/wpds/WPDS.hpp"
#include "wali/wpds/WpdsHandler.hpp"

#include "wali/wpds/ewpds/EWPDS.hpp"
#include "wali/wpds/ewpds/EWpdsHandler.hpp"

#include "wali/wpds/fwpds/FWPDS.hpp"
//#include "wali/wpds/ewpds/EWpdsHandler.hpp"

#include <xercesc/sax2/Attributes.hpp>

#include <iostream>
#include <fstream>


namespace wali
{

  const std::string QueryHandler::XMLTag("Query");
  const std::string QueryHandler::XMLPoststarTag("poststar");
  const std::string QueryHandler::XMLPrestarTag("prestar");

  QueryHandler::QueryHandler( WeightFactory& wf,MergeFnFactory* mf ) :
    wf(wf),
    mf(mf),
    currentHandler(NULL),
    isPrestar(false),
    pdsHandler(NULL),
    faHandler(NULL)
  {
    typeID = XMLString::transcode("type");
  }

  QueryHandler::QueryHandler( wpds::WpdsHandler* wpdsh , wfa::WfaHandler* wfah ) :
    wf(wfah->getWeightFactory()),
    mf(NULL),
    currentHandler(NULL),
    isPrestar(false),
    pdsHandler(wpdsh),
    faHandler(wfah)
  {
    typeID = XMLString::transcode("type");
  }

  QueryHandler::~QueryHandler()
  {
    XMLString::release(&typeID);
    delete pdsHandler;
    delete faHandler;
  }

  bool QueryHandler::queryIsPrestar() const
  {
    return isPrestar;
  }

  wfa::WfaHandler* QueryHandler::getWfaHandler()
  {
    return faHandler;
  }

  wpds::WpdsHandler* QueryHandler::getWpdsHandler()
  {
    return pdsHandler;
  }

  wfa::WFA& QueryHandler::result()
  {
    return fa;
  }

  wfa::WFA& QueryHandler::run()
  {
    try {
      if( isPrestar ) {
        //pdsHandler.get().prestar(faHandler.get(),fa);
        pdsHandler->get().prestar(faHandler->get(),fa);
      }
      else {
        //pdsHandler.get().poststar(faHandler.get(),fa);
        pdsHandler->get().poststar(faHandler->get(),fa);
      }
    } catch(...) {
      std::cerr << "[ERROR] QueryHandler::run() caught an exception.\n";
      std::cerr << "\tCurrent status of resultant WFA := \n";
      fa.print( std::cerr ) << std::endl;
      std::ofstream of("QH.run.result.dot");
      fa.print_dot(of);
      of.close();
    }
    return fa;
  }

  //////////////////////////////////////////////////
  // Parsing handlers
  //////////////////////////////////////////////////

  void QueryHandler::startDocument()
  {
    if (pdsHandler != NULL)
      pdsHandler->startDocument();
    if (faHandler != NULL)
      faHandler->startDocument();
  }

  void QueryHandler::endDocument()
  {
    static std::string perrmsg = "[ERROR] No *WPDS has been defined.";
    static std::string ferrmsg = "[ERROR] No WFA has been defined.";
    // Query must have a PDS
    if (pdsHandler == NULL) {
      *waliErr << perrmsg << std::endl;
      throw perrmsg;
    }
    // Query must have a WFA
    if (faHandler == NULL) {
      *waliErr << ferrmsg << std::endl;
      throw ferrmsg;
    }
    pdsHandler->endDocument();
    faHandler->endDocument();
  }

  void QueryHandler::startElement(  const   XMLCh* const    uri,
      const   XMLCh* const    localname,
      const   XMLCh* const    qname,
      const   Attributes&     attributes)
  {
    StrX who(localname);
    if( XMLTag == who.get() ) {
      StrX type = attributes.getValue(typeID);
      if( XMLPrestarTag == type.get() ) {
        isPrestar = true;
      }
      else if(XMLPoststarTag == type.get() ) {
        isPrestar = false;
      }
      else {
        assert(0);
      }
    }
    else {
      if (wfa::WFA::XMLTag == who.get()) {
        if (faHandler == NULL) {
          faHandler = new wfa::WfaHandler(wf);
        }
        currentHandler = faHandler;
      }
      else if (wpds::WPDS::XMLTag == who.get()) {
        if (pdsHandler == NULL) {
          pdsHandler = new wpds::WpdsHandler(wf);
        }
        currentHandler = pdsHandler;
      }
      else if (wpds::ewpds::EWPDS::XMLTag == who.get()) {
        if (pdsHandler == NULL) {
          wpds::ewpds::EWPDS* e = new wpds::ewpds::EWPDS();
          pdsHandler = new wpds::ewpds::EWpdsHandler(e,wf,mf);
        }
        currentHandler = pdsHandler;
      }
      else if (!currentHandler) {
        *waliErr << "[ERROR] In wali::QueryHandler, unhandled tag <> '" 
          << who << "'" << std::endl;
      }
      assert(currentHandler);
      currentHandler->startElement(uri,localname,qname,attributes);
    }
  }

  void QueryHandler::endElement( const XMLCh* const uri,
      const XMLCh* const localname,
      const XMLCh* const qname)
  {
    StrX who(localname);
    if( XMLTag == who.get() ) {
      // nothing todo
      currentHandler = 0;
    }
    else if( (wfa::WFA::XMLTag == who.get()) || (wpds::WPDS::XMLTag == who.get())) {
      currentHandler->endElement(uri,localname,qname);
      currentHandler = 0;
    }
    else {
      assert(currentHandler);
      currentHandler->endElement(uri,localname,qname);
    }
  }

  //
  // All characters should be white space
  //
  void QueryHandler::characters(const XMLCh* const chars, const unsigned int length)
  {
    if( currentHandler ) {
      currentHandler->characters(chars,length);
    }
    else {
      // do nothing. Get calls to characters for all white space 
      // b/w elements
    }
  }

  void QueryHandler::ignorableWhitespace(                               
      const XMLCh* const chars
      , const unsigned int length
      )
  {
    if( currentHandler ) {
      currentHandler->ignorableWhitespace(chars,length);
    }
  }

  void QueryHandler::processingInstruction(   
      const XMLCh* const target
      , const XMLCh* const data
      )
  {
    if( currentHandler ) {
      currentHandler->processingInstruction(target,data);
    }
  }

  //////////////////////////////////////////////////
  // Default error handlers
  //////////////////////////////////////////////////

  void QueryHandler::warning(const SAXParseException& exc)
  {
    StrX msg(exc.getMessage());
    std::cerr << "[WARNING] " << msg << std::endl;
  }

  void QueryHandler::error(const SAXParseException& exc)
  {
    StrX msg(exc.getMessage());
    std::cerr << "[ERROR] " << msg << std::endl;
  }

  void QueryHandler::fatalError(const SAXParseException& exc)
  {
    StrX msg(exc.getMessage());
    std::cerr << "[FATAL ERROR] " << msg << std::endl;
    throw exc;
  }

  //////////////////////////////////////////////////
  // Helpers
  //////////////////////////////////////////////////

} // namespace wali

