#include "wali/graph/InterGraph.hpp"
#include "wali/graph/IntraGraph.hpp"
#include "wali/graph/RegExp.hpp"

#include "wali/util/Timer.hpp"

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <iostream>
#include <iomanip>

// ::wali
#include "wali/SemElemTensor.hpp"

typedef int INTER_GRAPH_INT;

using namespace std;

namespace wali {

    namespace graph {

        UnionFind::UnionFind(int len) {
            n = len;
            count = len;
            arr = new int[n];
            for(int i=0; i < n;i++) {
                arr[i] = i;
            }
        }

        UnionFind::UnionFind(UnionFind& from) : n(from.n), count(from.count){
          arr = new int[n];
          for(int i=0; i<n; ++i)
            arr[i] = from.arr[i];
        }

        UnionFind::~UnionFind() {
            delete [] arr;
            arr = NULL;
            n = 0;
        }

        void UnionFind::reset() {
            for(int i=0; i < n;i++) {
                arr[i] = i;
            }
        }

        int UnionFind::find(int a) {
            int r = a,p;
            while(arr[r] != r) {
                r = arr[r];
            }
            // path compression
            while(arr[a] != a) {
                p = arr[a];
                arr[a] = r;
                a = p;
            }
            return r;
        }

        void UnionFind::takeUnion(int a, int b) {
            int ar = find(a);
            int br = find(b);
            arr[ar] = br; // FIXME: Randomize this
            //TODO: Used ranked union

            // if two sets were merged, the total number of sets has decreaseed
            // by one
            if(ar != br) count--;
            assert(count > 0 && "Count of sets reached zero: There is a bug\n");
        }

        int UnionFind::countSets(){
          return count;
        }

        std::ostream& UnionFind::print(std::ostream& o)
        {
          std::multiset< std::pair< int, int > > shadow;
          int curset = -1;

          o << "[ ";
          for(int i=0; i<n; ++i){
            shadow.insert(std::pair< int, int >(find(i),i));
          }

          for(std::multiset< std::pair< int, int > >::iterator iter = shadow.begin();
              iter != shadow.end();
              ++iter){
            if(iter->first > curset){
              if(curset != -1) 
                o << ">>>>>>>>>" << std::endl;
              curset = iter->first;
            }
            o << iter->second << " ";
          }
          o << "]";
          return o;
        }

        inter_node_t promote_type(inter_node_t t1, inter_node_t t2) {
            if(t1 == InterNone)
                return t2;
            if(t2 == InterNone)
                return t1;
            if(t1 == t2)
                return t1;
            return InterSourceOutNode;
        }

        bool is_source_type(inter_node_t t1) {
            return (t1 == InterSource || t1 == InterSourceOutNode);
        }

        void ETransHandler::addEdge(int call, int ret, sem_elem_t wtCallRule) {
          EdgeMap::iterator it = edgeMap.find(ret);
          if(it != edgeMap.end()) {
            Dependency &d = it->second;
            d.second = d.second->combine(wtCallRule);
          } else {
            edgeMap[ret] = Dependency(call, wtCallRule);
          }
        }

        bool ETransHandler::exists(int ret) {
          EdgeMap::iterator it = edgeMap.find(ret);
          if(it != edgeMap.end()) {
            return true;
          }
          return false;
        }


        // return NULL if no match found
        sem_elem_t ETransHandler::get_dependency(int ret, int &call) {
          EdgeMap::iterator it = edgeMap.find(ret);
          if(it != edgeMap.end()) {
            Dependency &d = it->second;
            call = d.first;
            return d.second;
          }
          return sem_elem_t(0);
        }

        InterGraph::~InterGraph() {
            for(unsigned i = 0; i < nodes.size(); i++) {
                if(nodes[i].gr && intra_graph_uf->find(i) == (int)i) {
                    delete nodes[i].gr;
                }
            }
            if(intra_graph_uf) {
                delete intra_graph_uf;
            }
        }

        int InterGraph::nodeno(Transition &t) {
            TransMap::iterator it = node_number.find(t);
            if(it == node_number.end()) {
                node_number[t] = static_cast<INTER_GRAPH_INT>(nodes.size());
                nodes.push_back(GraphNode(t));
                return (static_cast<INTER_GRAPH_INT>(nodes.size()) - 1);
            }
            return it->second;
        }

        bool InterGraph::exists(Transition &t) {
            TransMap::iterator it = node_number.find(t);
            if(it == node_number.end()) {
                return false;
            }
            return true;
        }

        int InterGraph::intra_edgeno(Transition &src, Transition &tgt) {
            int s = nodeno(src);
            int t = nodeno(tgt);
            std::list<int>::iterator it = nodes[s].outgoing.begin();
            for(; it != nodes[s].outgoing.end(); it++) {
                if(intra_edges[*it].tgt == t) {
                    return *it;
                }
            }
            return -1;
        }

        int InterGraph::inter_edgeno(Transition &src1, Transition &src2, Transition &tgt) {
            int s1 = nodeno(src1);
                int s2 = nodeno(src2);
                int t = nodeno(tgt);
                std::list<int>::iterator it = nodes[s2].out_hyper_edges.begin();
                for(; it != nodes[s2].out_hyper_edges.end(); it++) {
                    if(inter_edges[*it].tgt == t && inter_edges[*it].src1 == s1) {
                        return *it;
                    }
                }
                return -1;
            }

            bool InterGraph::exists(int state, int stack, WT_CHECK op) {
                size_t i;
                for(i=0;i<nodes.size();i++) {
                    if(static_cast<INTER_GRAPH_INT>(nodes[i].trans.src) == state
                       && static_cast<INTER_GRAPH_INT>(nodes[i].trans.stack) == stack)
                    {
                        if(op((nodes[i].gr->get_weight(nodes[i].intra_nodeno)).get_ptr())) 
                            return true;
                    }
                }
                return false;
            }

            std::ostream &InterGraph::print(std::ostream &out, PRINT_OP pop) {
                unsigned i;
                if(CONSTANT_CONDITION(1))//intra_graph_uf == NULL) 
                {
                    out << "Source Transitions:\n";
                    for(i=0;i<nodes.size();i++) {
                        if(is_source_type(nodes[i].type)) {
                            IntraGraph::print_trans(nodes[i].trans, out, pop);
                            nodes[i].weight->print(out);
                            out << "\n";
                        }
                    }
                    out << "IntraEdges:\n";
                    for(i=0;i<intra_edges.size();i++) {
                        Transition src = nodes[intra_edges[i].src].trans;
                        Transition tgt = nodes[intra_edges[i].tgt].trans;
                        IntraGraph::print_trans(src, out, pop);
                        out << "-->";
                        IntraGraph::print_trans(tgt, out, pop);
                        intra_edges[i].weight->print(out);
                        out << "\n";
                    }
                } else {
                    for(i = 0; i < nodes.size(); i++) {
                        if(nodes[i].gr && intra_graph_uf->find(i) == (int)i) {
                            nodes[i].gr->print(out,pop);
                            out << "\n";
                        }
                    }
                }
                out << "HyperEdges:\n";
                for(i = 0; i < inter_edges.size(); i++) {
                    Transition src1 = nodes[inter_edges[i].src1].trans;
                    Transition src2 = nodes[inter_edges[i].src2].trans;
                    Transition tgt = nodes[inter_edges[i].tgt].trans;
                    IntraGraph::print_trans(src1, out, pop);
                    out << ",";
                    IntraGraph::print_trans(src2, out, pop);
                    out << "-->";
                    IntraGraph::print_trans(tgt, out, pop);
                    if(inter_edges[i].weight.get_ptr()) {
                        inter_edges[i].weight->print(out);
                    } else {
                        inter_edges[i].mf->print(out);
                    }
                    out << "\n";
                }
                return out;
            }

            void InterGraph::addEdge(Transition src, Transition tgt, wali::sem_elem_t se) {
                int eno = intra_edgeno(src,tgt);
                if(eno != -1) { // edge already present
                    intra_edges[eno].weight = intra_edges[eno].weight->combine(se);
                    return;
                }
                int s = nodeno(src);
                int t = nodeno(tgt);
                GraphEdge ed(s,t,se);
                intra_edges.push_back(ed);
                int e = intra_edges.size() - 1;
                nodes[s].outgoing.push_back(e);
                nodes[t].incoming.push_back(e);
            }

            void InterGraph::addCallRetEdge(Transition src, Transition tgt, wali::sem_elem_t se) {
              addEdge(src, tgt, se->one());
              int s = nodeno(src);
              int t = nodeno(tgt);
              eHandler.addEdge(s, t, se);
            }

            void InterGraph::addEdge(Transition src1, Transition src2, Transition tgt, wali::sem_elem_t se) {
                int eno = inter_edgeno(src1,src2,tgt);
                if(eno != -1) { // edge already present
                    inter_edges[eno].weight = inter_edges[eno].weight->combine(se);
                    return;
                }
                int s1 = nodeno(src1);
                int s2 = nodeno(src2);
                int t = nodeno(tgt);
                HyperEdge ed(s1,s2,t,se);
                nodes[s2].type = promote_type(nodes[s2].type, InterOutNode);
                inter_edges.push_back(ed);
                nodes[s2].out_hyper_edges.push_back(inter_edges.size() - 1);
            }

            void InterGraph::addEdge(Transition src1, Transition src2, Transition tgt, merge_fn_t mf) {
                assert(running_ewpds);
                int eno = inter_edgeno(src1,src2,tgt);
                if(eno != -1 && mf == inter_edges[eno].mf) { // edge already present
                    return;
                }
                int s1 = nodeno(src1);
                int s2 = nodeno(src2);
                int t = nodeno(tgt);
                HyperEdge ed(s1,s2,t,mf);
                nodes[s2].type = promote_type(nodes[s2].type, InterOutNode);
                inter_edges.push_back(ed);
                nodes[s2].out_hyper_edges.push_back(inter_edges.size() - 1);
            }

            void InterGraph::addCallEdge(Transition src1, Transition src2) {
                call_edges.push_back(call_edge_t(nodeno(src1),nodeno(src2)));
            }

            void InterGraph::setSource(Transition t, wali::sem_elem_t se) {
                int n = nodeno(t);
                nodes[n].type = promote_type(nodes[n].type, InterSource);
                nodes[n].weight = se;
            }

            void InterGraph::setESource(Transition t, wali::sem_elem_t wtAtCall, wali::sem_elem_t wtAfterCall) {
                // setSource
                int n = nodeno(t);
                nodes[n].type = promote_type(nodes[n].type, InterSource);
                nodes[n].weight = wtAtCall;
                // Extra dependency
                eHandler.addEdge(-1, n, wtAfterCall);
            }

            unsigned InterGraph::SCC(list<IntraGraph *> &grlist, std::list<IntraGraph *> &grsorted) {
                std::list<IntraGraph *>::iterator gr_it;
                // reset visited
                for(gr_it = grlist.begin(); gr_it != grlist.end(); gr_it++) {
                    (*gr_it)->visited = false;
                    (*gr_it)->scc_number = 0;
                    (*gr_it)->bfs_number = (unsigned)(-1);
                }
                std::list<IntraGraph *> finished;
                std::map<IntraGraph *, std::list<IntraGraph *> > rev_edges;
                // Do DFS
                std::list<IntraGraph *>::reverse_iterator gr_rit;
                for(gr_rit = grlist.rbegin(); gr_rit != grlist.rend(); gr_rit++) {
                    IntraGraph *gr = *gr_rit;
                    if(gr->visited)
                        continue;
                    dfsIntraForward(gr,finished,rev_edges);
                }

                unsigned scc = 0;

                for(gr_it = grlist.begin(); gr_it != grlist.end(); gr_it++) {
                    (*gr_it)->visited = false;
                }

                for(gr_it = finished.begin(); gr_it != finished.end(); gr_it++) {
                    IntraGraph *gr = *gr_it;
                    if(gr->visited)
                        continue;
                    scc++;
                    typedef pair<IntraGraph *, std::list<IntraGraph *>::iterator> StackEl;
                    std::list<StackEl> stack;
                    stack.push_back(StackEl(gr, rev_edges[gr].begin()));
                    while(!stack.empty()) {
                        StackEl p = stack.front();
                        stack.pop_front();
                        IntraGraph *v = p.first;
                        std::list<IntraGraph *>::iterator it = p.second;
                        v->scc_number = scc;
                        if(!v->visited) grsorted.push_back(v);
                        v->visited = true;
                        while(it != rev_edges[v].end()) {
                            IntraGraph *c = *it;
                            if(c->visited) {
                                it++;
                            } else { 
                                stack.push_front(StackEl(v,++it));
                                stack.push_front(StackEl(c, rev_edges[c].begin()));
                                break;
                            }
                        }
                    }
                }
                // reset visited
                for(gr_it = grlist.begin(); gr_it != grlist.end(); gr_it++) {
                    (*gr_it)->visited = false;
                }
                return scc;
            }

            void InterGraph::dfsIntraForward(IntraGraph *gr, std::list<IntraGraph *> &finished, std::map<IntraGraph *, std::list<IntraGraph *> > &rev_edges) {
                gr->visited = true;
                std::list<int> *outnodes = gr->getOutTransitions();
                std::list<int>::iterator it;
                for(it = outnodes->begin(); it != outnodes->end(); it++) {
                    int n = *it;
                    std::list<int>::iterator beg = nodes[n].out_hyper_edges.begin();
                    std::list<int>::iterator end = nodes[n].out_hyper_edges.end();
                    for(; beg != end; beg++) {
                        IntraGraph *ch = nodes[inter_edges[*beg].tgt].gr;
                        rev_edges[ch].push_back(gr);
                        if(!ch->visited)
                            dfsIntraForward(ch, finished,rev_edges);
                    }
                }
                finished.push_front(gr);
            }

            // BFS of a SCC
            void InterGraph::bfsIntra(IntraGraph *start, unsigned int scc_number) {
                std::list <IntraGraph *> workset;
                workset.push_back(start);
                IntraGraph *gr;
                start->bfs_number = 0;
                while(!workset.empty()) {
                    gr = workset.front();
                    workset.pop_front();
                    if(gr->visited) continue;

                    gr->visited = true;

                    std::list<int> *outnodes = gr->getOutTransitions();
                    std::list<int>::iterator it;
                    for(it = outnodes->begin(); it != outnodes->end(); it++) {
                        int n = *it;
                        std::list<int>::iterator beg = nodes[n].out_hyper_edges.begin();
                        std::list<int>::iterator end = nodes[n].out_hyper_edges.end();
                        for(; beg != end; beg++) {
                            IntraGraph *ch = nodes[inter_edges[*beg].tgt].gr;
                            if(ch->scc_number != scc_number) continue;
                            if(!ch->visited) {
                                ch->bfs_number = (ch->bfs_number > (gr->bfs_number) + 1) ? (gr->bfs_number+1) : ch->bfs_number;
                                workset.push_back(ch);
                            }
                        }
                    }
                }
            }

            // BFS of a SCC
            void InterGraph::resetSCCedges(IntraGraph *gr, unsigned int scc_number) {
                std::list<int> *outnodes = gr->getOutTransitions();
                std::list<int>::iterator it;
                for(it = outnodes->begin(); it != outnodes->end(); it++) {
                    int n = *it;
                    std::list<int>::iterator beg = nodes[n].out_hyper_edges.begin();
                    std::list<int>::iterator end = nodes[n].out_hyper_edges.end();
                    for(; beg != end; beg++) {
                        int inode = inter_edges[*beg].tgt;
                        int onode1 = inter_edges[*beg].src1;
                        IntraGraph *ch = nodes[inode].gr;
                        if(ch->scc_number != scc_number) continue;
                        ch->updateEdgeWeight(nodes[onode1].intra_nodeno, nodes[inode].intra_nodeno, sem->zero());
                    }
                }
            }

            void InterGraph::setup_worklist(list<IntraGraph *> &gr_sorted, std::list<IntraGraph *>::iterator &gr_it, unsigned int scc_n,
                    multiset<tup> &worklist) {
                worklist.clear();
                while(gr_it != gr_sorted.end() && (*gr_it)->scc_number == scc_n ) {
                    std::list<int>::iterator tbeg = (*gr_it)->getOutTransitions()->begin();
                    std::list<int>::iterator tend = (*gr_it)->getOutTransitions()->end();
                    while(tbeg != tend) {
                        int onode = *tbeg;
                        worklist.insert(tup((*gr_it)->bfs_number,onode));
                        //nodes[onode].weight = nodes[onode].gr->get_weight(nodes[onode].trans);
                        tbeg++;
                    }
                    gr_it++;
                }
            }                       

            int myrand(int l, int u) {
                int a = rand();
                return l+int((u-l+1)*(a/(RAND_MAX + 1.0))); 
            }

#if 0
            std::ostream& _printArr2D(std::ostream& o, int **arr, int numRows, int cols[])
            {
              //You might want to have a newline before you call _printArr2D
              for(int i=0; i<numRows; ++i){
                for(int j =0; j<cols[i]; ++j){
                  o << arr[i][j] << " ";
                }
                o << std::endl;
              }
              return o;
            }

            std::ostream& _printArr1D(std::ostream& o, int arr[], int maxCols)
            {
              for(int i=0; i<maxCols; ++i)
                o << arr[i] << std::endl;
              return o;
            }
#endif //#if 0

            // I call this raw dfs because there are some dfs implementations floating around.
            // Mine works directly on arrays storing the adjacency list.
            // Note: This implementation is highly tailored to the use case (see below)
            // Time Complexity: O(#nodes *max{out-degree})
            // out: scc stores sccs in sets
            // out: mindfsn also holds a topological order of connected components
            void InterGraph::rawDfs(
                const int u, //current node.
                
                const int deg[], //[i]: degree of node i
                const std::vector< std::vector< int > > adjMat, //The adjacency matrix
                
                int dfsn[], //[i]: the dfs number of vertex i
                int& dfsnext, //next free dfs number
                int comp[], //O(1) membership stack containing the 
                            //vertices of current component
                int& ncomp, //number of outstanding vertices in the components
                bool incomp[], //[i] a marker that says, I've seen i, but haven't 
                              //finished putting it in a component

                int mindfsn[], //(in:out) [i]: minimum dfs number reachable from vertex i
                UnionFind& scc //(out) The output scc are stored here.
                )
            { 
              //If this does not make any sense to you
              // go to: http://shygypsy.com/tools/scc.cpp
              //The code below is an adapted version.

              dfsn[u] = mindfsn[u] = dfsnext++;
              comp[ncomp++] = u;
              incomp[u] = true;
             
              for(int i = 0, v; i < deg[u]; ++i){
                v = adjMat[u][i];
                if(u == v)
                  //I really don't care about self loops
                  continue;
                if(dfsn[v] == -1){
                  rawDfs(v,deg,adjMat,dfsn,dfsnext,comp,ncomp,incomp,mindfsn,scc);
                  mindfsn[u] = (mindfsn[v] < mindfsn[u])? mindfsn[v] : mindfsn[u];
                }else if(incomp[v]){
                  //mindfsn[u] = (mindfsn[v] < mindfsn[u])? mindfsn[v] : mindfsn[u];
                  mindfsn[u] = (dfsn[v] < mindfsn[u])? dfsn[v] : mindfsn[u];
                }
              }

              if(dfsn[u] == mindfsn[u]){
                //u is the root of a connected component. Unify the component
                //and forget it.
                do{
                  scc.takeUnion(u, comp[--ncomp]);
                  incomp[comp[ncomp]] = false;
                }while(comp[ncomp] != u);
              }
              //else I've seen a smaller mindfsn in my dfs from this node
              //i.e., This node belongs to some smaller numbered scc. So, I will
              //be added to an SCC when that recursive call returns.
            }


            // Creates a bunch of intergraphs and uses Nodelisting to generate
            // regular expressions.  Then executes Newton iterations till
            // stabilization.  Since we use lazy transitions (and lazy
            // RegExps), only needed values are computed, other values can be
            // computed on demand.
            void InterGraph::setupNewtonSolution()
            {
              //Having this lying around as non tensor can do harm*/
              sem_elem_tensor_t sem_old = dynamic_cast<SemElemTensor*>(sem.get_ptr());
              sem = sem_old->tensor(sem_old.get_ptr());

              RegExp::startSatProcess(sem);

              //Why is this implementation so myopic? Because it has to fit
              //in this large framework which doesn't give me  nice hooks 
              //to do stuff. The idea is that this function along with rawDfs
              //are almost independent of the rest of the code.
              bool dbg = true;
              std::ostream& o = std::cout;

              int n = nodes.size(); //total number of nodes in the graph
              vector< GraphEdge >::iterator geIter;
              vector< HyperEdge >::iterator heIter;
              std::list<IntraGraph *>::iterator grIter;

              //First, find the CFGs
              UnionFind cfgUf(n); //The cfgs are constructed as sets cfgUf
              for(
                  geIter = intra_edges.begin();
                  geIter != intra_edges.end();
                  ++geIter){
                cfgUf.takeUnion(geIter->src,geIter->tgt);
                //if(dbg) 
                //  o << "Intra:" << geIter->src 
                //  << "->" << geIter->tgt << endl;
              }
              for(
                  heIter = inter_edges.begin();
                  heIter != inter_edges.end();
                  ++heIter){
                cfgUf.takeUnion(heIter->src1,heIter->tgt);
                //if(dbg) 
                //  o << "Inter:i:" << heIter->src1 
                //  << ",e:" << heIter->src2 
                //  << "->" << heIter->tgt << endl;
              }
              //cfgUf now has sets corresponding to the CFGs.

              if(dbg){
                o << "cfgUf after the initial cfg finding: ";
                cfgUf.print(o) << std::endl;
              }

              //int nCfg = cfgUf.countSets(); //Count the number of CFGs - O(1)

              /////Next, find SCCs

              //We will now renumber the target nodes so as to have them
              //numberd consecutively.
              //Time Complexity: O(#TgtNodes)
              int nTgt = inter_edges.size(); //total number of target nodes
              int tgt2Node[nTgt]; //map from 1...nTgt to actual tgt node
              int tgt2ExtSrc[nTgt]; //the external source for each target
              int tgt2IntSrc[nTgt]; //the internal source for each target
              int tgt2HypEdge[nTgt];
              int i;
              for(
                  i = 0,
                  heIter = inter_edges.begin();
                  heIter != inter_edges.end();
                  ++i,
                  ++heIter){
                tgt2Node[i] = heIter->tgt;
                tgt2ExtSrc[i] = heIter->src2;
                tgt2IntSrc[i] = heIter->src1;
                //whacko!
                tgt2HypEdge[i] = i;
              }

              if(dbg){
                o << "Consecute numbers assigned to target nodes. Here are the maps:" << endl;
                o << "tgt2Node: ";
                for(int i=0; i<nTgt; ++i)
                  o << i << ":" << tgt2Node[i] << " ";
                o << endl;
                o << "tgt2ExtSrc: ";
                for(int i=0; i<nTgt; ++i)
                  o << i  << ":" << tgt2ExtSrc[i] << " ";
                o << endl;
                o << "tgt2IntSrc: ";
                for(int i=0; i<nTgt; ++i)
                  o << i << ":" << tgt2IntSrc[i] << " ";
                o << endl;
              }

              //Compute a list of target nodes in each CFG
              //Time Complexity: O(#CFGs + #TgtNodes)
              //We tradeoff space for time -- use arrays everywhere.
              int lTgtNodes[n][nTgt]; //lists of targets per cfg
              int nTgtNodes[n]; //number of target nodes per cfg
              for(int i=0; i<n; ++i){
                nTgtNodes[i]=0;
              }
              if(dbg){
                //Makes debugging easier. All unused
                //space is -1. We don't want this
                //in production code because of
                //complexity
                for(int i=0; i < n; ++i)
                  for(int j=0; j < nTgt; ++j)
                    lTgtNodes[i][j] = -1;
              }
              for(int i=0; i < nTgt; ++i){
                int tgtCfg = cfgUf.find(tgt2Node[i]);
                lTgtNodes[tgtCfg][nTgtNodes[tgtCfg]++] = i;
              }
              if(dbg){
                o << "List of target nodes in each CFG. [Using real node numbers]" << endl;
                for(int i=0; i<n; ++i){
                  if(nTgtNodes[i] != 0)
                    o << i << " --> ";
                  for(int j=0; j<nTgtNodes[i];++j){
                    o << tgt2Node[lTgtNodes[i][j]] << " ";
                  }
                  if(nTgtNodes[i] != 0)
                    o << endl;
                }
                o << "------" << endl;
              }

              //Create the adjacency matrix.
              //uses consecutive numbering.
              //Time Complexity: O(#TgtNodes^2) or O(#call_edges)

              // There is no clean way of passing this to a function if this is
              // na array, hence pionter
              // int dependents[nTgt][nTgt];
              std::vector< std::vector< int > > dependents;
              for(int i=0; i<nTgt; ++i){
                std::vector< int > nv;
                dependents.push_back(nv);
              }
                
              int nDependents[nTgt];
              for(int i = 0; i< nTgt; ++i){
                nDependents[i] = 0;
              }
              for(int i = 0; i < nTgt; ++i){
                int c;
                //add me as a dependent to all targets in my CFG.
                c = cfgUf.find(tgt2Node[i]);
                for(int j = 0; j < nTgtNodes[c]; ++j){
                  int other = lTgtNodes[c][j];
                  dependents[other].push_back(i);
                  nDependents[other]++;
                }
                //add me as a dependent to all targets in the CFG of 
                //my external source
                c = cfgUf.find(tgt2ExtSrc[i]);
                for(int j = 0; j < nTgtNodes[c]; ++j){
                  int other = lTgtNodes[c][j];
                  dependents[other].push_back(i);
                  nDependents[other]++;
                }
              }

              if(dbg){
                o << "Adjacency matrix computed. [Using real node numbers]" << endl;
                for(int i=0; i<nTgt; ++i){
                  o << tgt2Node[i] << " --> ";
                  for(int j=0; j < nDependents[i]; ++j){
                    o << tgt2Node[dependents[i][j]] << " ";
                  }
                  o << endl;
                }
                o << "------" << endl;
              }

              //Now the actual SCC. The following are used by rawDfs
              UnionFind sccUf(nTgt); //output: sets will represent SCCs
              int dfsn[nTgt]; 
              int dfsnext;
              int mindfsn[nTgt];
              int comp[nTgt];
              int ncomp;
              bool incomp[nTgt];

              dfsnext = 0;
              ncomp = 0;
              for(int i=0; i < nTgt; ++i){
                dfsn[i] = -1;
                mindfsn[i] = -1; // Not really needed
                comp[i] = 0;
                incomp[i] = false;
              }
              for(int u=0; u < nTgt; ++u){
                if(dfsn[u] == -1)
                  rawDfs(
                      u,
                      nDependents,dependents,
                      dfsn,dfsnext,comp,ncomp,incomp,
                      mindfsn,sccUf
                      );
              }

              if(dbg){
                o << "SCC done. SCCs:" << endl;
                  for(int i=0; i < nTgt; ++i){
                    o << "SCC[" << i <<"]: ";
                    for(int j=0; j<nTgt; ++j){
                      if(mindfsn[j] == i){
                        o << tgt2Node[j] << " ";
                      }
                    }
                    o << endl;
                  }
                o << "------" << endl;
              }
              // SCC done. sccUf now has sets that corresponds to SCCs (with
              // targets numbered from 0...nTgt-1)
              // Note: mindfsn holds a topological order on connected
              // components (while SCCs have the same mindfsn)

              //Setup worklist.
              //- We use multiset<mindfsn,tgt> for worklist
              //  This gives us a worklist that is always sorted by mindfsn, hence
              //  our fixpoint operation proceeds in topological order.
              WorkList wl;
              for(int i=0; i < nTgt; ++i)
                wl.insert(tup(mindfsn[i],i));

              if(dbg){
                o << "Worklist:";
                for(WorkList::const_iterator iter = wl.begin(); iter != wl.end(); ++iter){
                  o << "[" << iter->first << "," << tgt2Node[iter->second] <<"] ";
                }
                o << endl;
              }

              // We update cfgUf to those we get after differentiation, i.e.,
              // call graphs are merged into CFGs
              for(int i=0; i<nTgt; ++i){
                cfgUf.takeUnion(tgt2Node[i],tgt2ExtSrc[i]);
              }

              // Now create the IntraGraphs
              std::list<IntraGraph *>::iterator gr_it;
              for(int i = 0; i < n;i++) {
                int j = cfgUf.find(i);
                if(nodes[j].gr == NULL) {
                  nodes[j].gr = new IntraGraph(running_prestar,sem);
                  gr_list.push_back(nodes[j].gr);
                }
                nodes[i].gr = nodes[j].gr;

                nodes[i].intra_nodeno = nodes[i].gr->makeNode(nodes[i].trans);

                if(is_source_type(nodes[i].type)) {
                  //This is a source node. 
                  //Pre*:   w -> (w^T,1)
                  //Post*:  w -> (1^T,w)
                  //where ^T is transpose and (,) is tensor
                  sem_elem_tensor_t wt = dynamic_cast<SemElemTensor*>((nodes[i].weight).get_ptr());
                  sem_elem_tensor_t one = dynamic_cast<SemElemTensor*>((wt->one()).get_ptr());
                  if(running_prestar){
                    wt = wt->transpose();
                    wt = wt->tensor(one.get_ptr());
                  }else{
                    wt = one->tensor(wt.get_ptr());
                  }
                  nodes[i].gr->setSource(nodes[i].intra_nodeno, wt);
                }
                // zero all weights (some are set by setSource() )
                // All weights should be tensored
                sem_elem_tensor_t zerot = dynamic_cast<SemElemTensor*>((sem->zero()).get_ptr()); //sem is tensored
                if(nodes[i].weight.get_ptr() != NULL)
                  nodes[i].weight = zerot;
              }

              //Pre*:
              //All edges in the intraprocedural graph get weights of the
              //form (w^T,1) where (,) denotes tensor, ^T denotes transpose,
              //and w is the weight on the CFG edge
              //Post*:
              //All edges in the intraprocedural graph get weights of the
              //form (1^T,w) 
              for(geIter = intra_edges.begin(); geIter != intra_edges.end(); ++geIter) {
                int s = geIter->src;
                int t = geIter->tgt;
                sem_elem_tensor_t wt = dynamic_cast<SemElemTensor*>((geIter->weight).get_ptr());
                sem_elem_tensor_t one = dynamic_cast<SemElemTensor*>(wt->one().get_ptr());
                if(running_prestar){
                  wt = wt->transpose();
                  wt = wt->tensor(one.get_ptr());
                }else{
                  wt = one->tensor(wt.get_ptr());
                }
                nodes[s].gr->addEdge(nodes[s].intra_nodeno, nodes[t].intra_nodeno, wt);
              }
              //For each hyperedge (tgt <- src1 src2], we add two edges [t <-
              //src1] and [t <- src2]. Initially these edges have the weight
              //zero.
              //These edges will be updatable.
              for(heIter = inter_edges.begin(); heIter != inter_edges.end(); heIter++) {
                IntraGraph *gr = nodes[heIter->tgt].gr;
                sem_elem_tensor_t zerot = dynamic_cast<SemElemTensor*>(sem->zero().get_ptr()); //zero is tensored
                gr->addEdge(nodes[heIter->src1].intra_nodeno, nodes[heIter->tgt].intra_nodeno, zerot, true);
                gr->addEdge(nodes[heIter->src2].intra_nodeno, nodes[heIter->tgt].intra_nodeno, zerot, true);
                //The external source and the target must belong to the same CFG after differentiation.
                assert(nodes[heIter->src2].gr == gr);
              }
              //Done creating IntraGraphs

              //setupIntraSolution uses path listing to create regular expressions for each CFG.
              for(grIter = gr_list.begin(); grIter != gr_list.end(); grIter++) {
                (*grIter)->setupIntraSolution(false);
              }
              if(dbg)
                o << "Intra setup done.\n";


              //Now Saturate Netwon's iterations.
              while(!wl.empty()) {
                bool changed = false;
                WorkList::iterator wit = wl.begin();

                if(dbg)
                  o << "Saturation: popped:" << tgt2Node[wit->second] << ",SCC=" << wit->first << endl;

                int tgt = (*wit).second;
                wl.erase(wit);

                sem_elem_tensor_t intWt, extWt, intEdgeWt, extEdgeWt, newIntEdgeWt, newExtEdgeWt;
                IntraGraph* gr = nodes[tgt2Node[tgt]].gr;

                intWt = dynamic_cast<SemElemTensor*>(gr->get_weight(nodes[tgt2IntSrc[tgt]].intra_nodeno).get_ptr());
                extWt = dynamic_cast<SemElemTensor*>(gr->get_weight(nodes[tgt2ExtSrc[tgt]].intra_nodeno).get_ptr());
                intEdgeWt = dynamic_cast<SemElemTensor*>(gr->readEdgeWeight(nodes[tgt2IntSrc[tgt]].intra_nodeno, nodes[tgt2Node[tgt]].intra_nodeno).get_ptr());
                extEdgeWt = dynamic_cast<SemElemTensor*>(gr->readEdgeWeight(nodes[tgt2ExtSrc[tgt]].intra_nodeno, nodes[tgt2Node[tgt]].intra_nodeno).get_ptr());

                if(!running_ewpds)
                  assert(false && "Newton's method only works for EWPDS right now!\n");

                //Does this even make sense? Can you save an iterator and then use
                //it as an index? 
                if(
                    false &&          //FIXME:SHORT CIRCUITED
                    running_ewpds && inter_edges[(tgt2HypEdge[tgt])].mf.get_ptr()){

                  assert(false && "Newton's method: I don't het know how to handle MergeFns.\n");

                }else{
                  {
                    //intEdgeWt
                    //D() denotes detensor
                    //Pre*: ([f(extSrc,tgtNode) X D(extWt)]^T , 1)
                    //Post*: (1, [f(extSrc,tgtNode) X D(extWt)])
                    sem_elem_tensor_t extWtD = extWt->detensorTranspose();
                    sem_elem_tensor_t wt = dynamic_cast<SemElemTensor*>(inter_edges[tgt2HypEdge[tgt]].weight.get_ptr());
                    if(wt == NULL)
                      wt = dynamic_cast<SemElemTensor*>(extWtD->one().get_ptr());
                      //FIXME:Should be this. Above is a part of short-circuit
                      //wt = dynamic_cast<SemElemTensor*>(extWtD->zero().get_ptr());
                    wt = dynamic_cast<SemElemTensor*>(wt->extend(extWtD.get_ptr()).get_ptr());
                    sem_elem_tensor_t one  = dynamic_cast<SemElemTensor*>((wt->one()).get_ptr());
                    if(running_prestar){
                      wt = wt->transpose();
                      wt = wt->tensor(one.get_ptr());
                    }else{
                      wt = one->tensor(wt.get_ptr());
                    }
                    if(!intEdgeWt->equal(wt.get_ptr())){
                      changed = true;
                      gr->updateEdgeWeight(nodes[tgt2IntSrc[tgt]].intra_nodeno, nodes[tgt2Node[tgt]].intra_nodeno, wt);
                    }
                  }
                  {
                    //extEdgeWt
                    //D() denotes detensor
                    //Pre*: ([f(extSrc,tgtNode)]^T, [D(intWt)]^T)
                    //Post*: ([D(intWt) X f(extSrc,tgtNode)], 1)
                    sem_elem_tensor_t intWtD = intWt->detensorTranspose();
                    sem_elem_tensor_t wt = dynamic_cast<SemElemTensor*>(inter_edges[tgt2HypEdge[tgt]].weight.get_ptr());
                    if(wt == NULL)
                      wt = dynamic_cast<SemElemTensor*>(intWtD->one().get_ptr());
                      //FIXME:Should be this. Above is a part of short-circuit
                      //wt = dynamic_cast<SemElemTensor*>(intWD->zero().get_ptr());
                    sem_elem_tensor_t one  = dynamic_cast<SemElemTensor*>((wt->one()).get_ptr());
                    if(running_prestar){
                      wt = wt->transpose();
                      wt = wt->tensor((intWtD->transpose()).get_ptr());
                    }else{
                      wt = dynamic_cast<SemElemTensor*>((intWtD->extend(wt.get_ptr())).get_ptr());
                      wt = wt->tensor(one.get_ptr());
                    }
                    if(!extEdgeWt->equal(wt.get_ptr())){
                      changed = true;
                      gr->updateEdgeWeight(nodes[tgt2ExtSrc[tgt]].intra_nodeno, nodes[tgt2Node[tgt]].intra_nodeno, wt);
                    }
                  }
                }

                if(changed){
                  for(int i=0; i < nDependents[tgt]; ++i){
                    wl.insert(tup(mindfsn[dependents[tgt][i]],dependents[tgt][i]));
                  }
                }

                if(dbg){
                  o << "Saturation: Worklist:";
                  for(WorkList::const_iterator iter = wl.begin(); iter != wl.end(); ++iter){
                    o << "[" << iter->first << "," << tgt2Node[iter->second] <<"] ";
                  }
                  o << endl;
                }
              }
              //Now I will set some class data members and globals that the
              //rest of the graph implementation seems to rely upon
              intra_graph_uf = new UnionFind(cfgUf);
              RegExp::stopSatProcess();
              RegExp::executingPoststar(!running_prestar);
            }


            // If an argument is passed in then only weights on those transitions will be available
            // I can fix this (i.e., weights for others will be available on demand), but not right now.
            void InterGraph::setupInterSolution(std::list<Transition> *wt_required) {
                RegExp::startSatProcess(sem);

                //util::Timer *timer = new util::Timer("FWPDS Find Graphs");

                // First, find the IntraGraphs
                int n = nodes.size();
                int i;
                unsigned int max_scc_required;
                intra_graph_uf = new UnionFind(n);

                vector<GraphEdge>::iterator it;
                vector<HyperEdge>::iterator it2;

                for(it = intra_edges.begin(); it != intra_edges.end(); it++) {
                    intra_graph_uf->takeUnion((*it).src,(*it).tgt);
                }


                for(it2 = inter_edges.begin(); it2 != inter_edges.end(); it2++) {
                    intra_graph_uf->takeUnion((*it2).src1,(*it2).tgt);
                }

                std::list<IntraGraph *>::iterator gr_it;

                for(i = 0; i < n;i++) {
                    int j = intra_graph_uf->find(i);
                    if(nodes[j].gr == NULL) {
                        nodes[j].gr = new IntraGraph(running_prestar,sem);
                        gr_list.push_back(nodes[j].gr);
                    }
                    nodes[i].gr = nodes[j].gr;

                    nodes[i].intra_nodeno = nodes[i].gr->makeNode(nodes[i].trans);

                    if(is_source_type(nodes[i].type)) {
                        nodes[i].gr->setSource(nodes[i].intra_nodeno, nodes[i].weight);
                    }
                    // zero all weights (some are set by setSource() )
                    if(nodes[i].weight.get_ptr() != NULL)
                        nodes[i].weight = nodes[i].weight->zero();

                }

                // Now fill up the IntraGraphs
                for(it = intra_edges.begin(); it != intra_edges.end(); it++) {
                    int s = (*it).src;
                    int t = (*it).tgt;
                    nodes[s].gr->addEdge(nodes[s].intra_nodeno, nodes[t].intra_nodeno, (*it).weight);
                }

                for(it2 = inter_edges.begin(); it2 != inter_edges.end(); it2++) {
                    IntraGraph *gr = nodes[(*it2).tgt].gr;
                    gr->addEdge(nodes[(*it2).src1].intra_nodeno, nodes[(*it2).tgt].intra_nodeno, sem->zero(), true);

                    IntraGraph *gr2 = nodes[(*it2).src2].gr;
                    gr2->setOutNode(nodes[(*it2).src2].intra_nodeno, (*it2).src2);
                }

                // For SWPDS
                vector<call_edge_t>::iterator it3;
                for(it3 = call_edges.begin(); it3 != call_edges.end(); it3++) {
                    IntraGraph *gr1 = nodes[(*it3).first].gr;
                    IntraGraph *gr2 = nodes[(*it3).second].gr;
                    gr1->addCallEdge(gr2);
                }

                // Setup Worklist
                multiset<tup > worklist;

#ifdef STATIC_MEMORY
                int max_size = 0;
                for(gr_it = gr_list.begin(); gr_it != gr_list.end(); gr_it++) {
                    max_size = (max_size > (*gr_it)->getSize()) ? max_size : (*gr_it)->getSize();
                }
                IntraGraph::addStaticBuffer(max_size);
#endif

                for(gr_it = gr_list.begin(); gr_it != gr_list.end(); gr_it++) {
                    (*gr_it)->setupIntraSolution(false);
                }
                //cout << "Intra setup done.\n";

                // Do SCC decomposition of IntraGraphs
                std::list<IntraGraph *> gr_sorted;
                unsigned components = SCC(gr_list, gr_sorted);
                STAT(stats.ncomponents = components);


                //delete timer;
                //timer = new util::Timer("FWPDS Saturation");

                // Saturate
                if(wt_required == NULL) {
                    max_scc_required = components;
                } else {
                    max_scc_required = 0;
                    std::list<Transition>::iterator trans_it;
                    for(trans_it = wt_required->begin(); trans_it != wt_required->end(); trans_it++) {
                        int nno = nodeno(*trans_it);
                        max_scc_required = (max_scc_required >= nodes[nno].gr->scc_number) ? max_scc_required : nodes[nno].gr->scc_number;
                    }
                }
                //cout << "Saturation started\n";
                gr_it = gr_sorted.begin();
                for(unsigned scc_n = 1; scc_n <= max_scc_required; scc_n++) {
                    //cout << ".";
                    bfsIntra(*gr_it, scc_n);
                    setup_worklist(gr_sorted, gr_it, scc_n, worklist);
                    saturate(worklist,scc_n);
                }
                max_scc_computed = max_scc_required;
                RegExp::stopSatProcess();
                RegExp::executingPoststar(!running_prestar);

#ifdef STATIC_MEMORY
                IntraGraph::clearStaticBuffer();
#endif
                //delete timer;
            }

            std::ostream &InterGraph::print_stats(std::ostream &out) {
                InterGraphStats total_stats = stats;
                int n = nodes.size();
                int i;      
                set<RegExp *> reg_equations;
                int ned = 0;
                for(i = 0; i < n; i++) {
                    if(intra_graph_uf->find(i) == i) {
                        std::list<int>::iterator tbeg = nodes[i].gr->getOutTransitions()->begin();
                        std::list<int>::iterator tend = nodes[i].gr->getOutTransitions()->end();
                        for(; tbeg != tend; tbeg++) {
                            int onode = nodes[i].intra_nodeno;
                            reg_equations.insert(nodes[i].gr->nodes[onode].regexp.get_ptr());
                        }
                        total_stats.ngraphs ++;
                        IntraGraphStats st = nodes[i].gr->get_stats(); 
                        total_stats.ncombine += st.ncombine;
                        total_stats.nextend += st.nextend;
                        total_stats.nstar += st.nstar;
                        total_stats.nupdatable += st.nupdatable * st.nupdatable;
                        total_stats.ncutset += st.ncutset * st.ncutset * st.ncutset;
                        total_stats.nget_weight += st.nget_weight;
                        total_stats.ndom_sequence += st.ndom_sequence;
                        total_stats.ndom_components += st.ndom_components;
                        total_stats.ndom_componentsize += (st.ndom_componentsize * st.ndom_componentsize);
                        total_stats.ndom_componentcutset += st.ndom_componentcutset;
                        ned += st.nedges;
                    }
                }
                int changestat = RegExp::out_node_height(reg_equations);
                total_stats.nhyperedges = inter_edges.size();
                total_stats.nedges = intra_edges.size();
                total_stats.nnodes = nodes.size();

                RegExpStats rst = RegExp::get_stats();
                total_stats.ncombine += rst.ncombine;
                total_stats.nextend += rst.nextend;
                total_stats.nstar += rst.nstar;
                total_stats.ngraphs = (total_stats.ngraphs == 0) ? 1 : total_stats.ngraphs;
                rst.out_nodes = (rst.out_nodes == 0) ? 1 : rst.out_nodes;
                out << "----------------------------------\n";
                out << "          FWPDS Stats             \n";
                out << "----------------------------------\n";
                out << "InterGraph nodes : " << total_stats.nnodes << "\n";
                out << "InterGraph edges : " << total_stats.nedges << "\n";
                out << "InterGraph hyperedges : " << total_stats.nhyperedges << "\n";
                out << "InterGraph iterations : " << total_stats.niter << "\n";
                out << "InterGraph get_weight : " << total_stats.nget_weight << "\n";
                out << "IntraGraphs : " << total_stats.ngraphs << "\n";
                out << "IntraGraph SCC : " << total_stats.ncomponents << "\n";
                out << "IntraGraph SCC Computed: " << max_scc_computed << "\n";
                out << "Avg. IntraGraph nodes : " << (total_stats.nnodes / total_stats.ngraphs) << "\n";
                out << "Avg. IntraGraph edges : " << (ned / total_stats.ngraphs) << "\n";
                out << "Avg. IntraGraph cutset : " << (pow(total_stats.ncutset/total_stats.ngraphs,0.33)) << "\n";
                out << "Avg. IntraGraph updatable : " << (int)(pow((double)total_stats.nupdatable/total_stats.ngraphs,0.5)) << "\n";
                out << "Avg. IntraGraph dom-sequence length : " << (total_stats.ndom_sequence / total_stats.ngraphs) << "\n";
                out << "Avg. IntraGraph dom-component size : " << (int)pow((double)total_stats.ndom_componentsize / (total_stats.ndom_components+1),0.5) << "\n";
                out << "Avg. IntraGraph dom-component cuset : " << setprecision(2) << (double)total_stats.ndom_componentcutset / (total_stats.ndom_components+1) << "\n";
                out << "Semiring Combine : " << total_stats.ncombine << "\n";
                out << "Semiring Extend : " << total_stats.nextend << "\n";
                out << "Semiring Star : " << total_stats.nstar << "\n";
                out << "RegExp HashMap hits : " << rst.hashmap_hits << "\n";
                out << "RegExp HashMap misses : " << rst.hashmap_misses << "\n";
                out << "OutNode Height : " << setprecision(4) << (rst.height / rst.out_nodes) << "\n"; 
                out << "OutNode Loop ND : " << setprecision(4) << (rst.lnd / rst.out_nodes) << "\n";
                out << "Change Stat : " << changestat << "\n";
                out << "\n";
                return out;
            }

            // New Saturation Procedure -- minimize calls to get_weight
            void InterGraph::saturate(multiset<tup> &worklist, unsigned scc_n) {
              sem_elem_t weight;
              std::list<int> *moutnodes;

                while(!worklist.empty()) {
                    // Get an outnode whose weight is to be propagated
                    multiset<tup>::iterator wit = worklist.begin();
                    int onode = (*wit).second;
                    worklist.erase(wit);
                    //int onode = worklist.front();
                    //worklist.pop_front();

                    weight = nodes[onode].gr->get_weight(nodes[onode].intra_nodeno);
                    if(nodes[onode].weight.get_ptr() != NULL && nodes[onode].weight->equal(weight))
                        continue;
                    nodes[onode].weight = weight;

                    STAT(stats.niter++);

                    FWPDSDBGS(
                            cout << "Popped ";
                            IntraGraph::print_trans(nodes[onode].trans,cout) << "with weight ";
                            weight->print(cout) << "\n";
                            );

                    // Go through all its targets and modify their weights
                    std::list<int>::iterator beg = nodes[onode].out_hyper_edges.begin();
                    std::list<int>::iterator end = nodes[onode].out_hyper_edges.end();
                    for(; beg != end; beg++) {
                        int inode = inter_edges[*beg].tgt;
                        int onode1 = inter_edges[*beg].src1;
                        sem_elem_t uw;
                        if(running_ewpds && inter_edges[*beg].mf.get_ptr()) {
                            uw = inter_edges[*beg].mf->apply_f(sem->one(), weight);
                            FWPDSDBGS(
                                    cout << "Apply merge function ";
                                    inter_edges[*beg].mf->print(cout) << " to ";
                                    weight->print(cout) << "\n";
                                    uw->print(cout << "Got ") << "\n";
                                    );
                        } else {
                            uw = inter_edges[*beg].weight->extend(weight);
                        }
                        STAT(stats.nextend++);
                        nodes[inode].gr->updateEdgeWeight(nodes[onode1].intra_nodeno, nodes[inode].intra_nodeno, uw);
                    }
                    // Go through all targets again and insert them into the workist without
                    // seeing if they actually got modified or not
                    beg = nodes[onode].out_hyper_edges.begin();
                    for(; beg != end; beg++) {
                        int inode = inter_edges[*beg].tgt;
                        IntraGraph *gr = nodes[inode].gr;
                        if(gr->scc_number != scc_n) {
                            assert(gr->scc_number > scc_n);
                            continue;
                        }
                        moutnodes = gr->getOutTransitions();
                        std::list<int>::iterator mbeg = moutnodes->begin();
                        std::list<int>::iterator mend = moutnodes->end();
                        for(; mbeg != mend; mbeg++) {
                            int mnode = (*mbeg);
                            worklist.insert(tup(gr->bfs_number, mnode));
                        }
                    }
                }
            }

            // Must be called after saturation
            sem_elem_t InterGraph::get_call_weight(Transition t) {
                unsigned orig_size = nodes.size();
                unsigned n = nodeno(t);
                assert(orig_size == nodes.size()); // Transition t must not be a new one

                return nodes[n].gr->get_weight(nodes[n].intra_nodeno);
            }

            sem_elem_t InterGraph::get_weight(Transition t) {
                unsigned orig_size = nodes.size();
                unsigned n = nodeno(t);
                assert(orig_size == nodes.size()); // Transition t must not be a new one

                // check eHandler
                if(eHandler.exists(n)) {
                  // This must be a return transition
                  int nc;
                  sem_elem_t wtCallRule = eHandler.get_dependency(n, nc);
                  sem_elem_t wt;
                  if(nc != -1) {
                    wt = nodes[nc].gr->get_weight(nodes[nc].intra_nodeno);
                  } else {
                    // ESource
                    wt = sem->one();
                  }
                  return wt->extend(wtCallRule);
                }

                return nodes[n].gr->get_weight(nodes[n].intra_nodeno);
            }

            void InterGraph::update_all_weights() {
                unsigned int i;
                for(i=0;i<nodes.size();i++) {
                    sem_elem_t w = nodes[i].gr->get_weight(nodes[i].intra_nodeno);
                    nodes[i].weight = w;
                }
            }

            inline int get_number(map<int,int> &intra_node_map, int src, IntraGraph *ca) {
                std::map<int,int>::iterator it = intra_node_map.find(src);
                if(it != intra_node_map.end()) {
                    return it->second;
                }
                int s = ca->makeNode();
                intra_node_map[src] = s;
                return s;
            }

            bool InterGraph::path_summary(int state, int stack, int accept, WT_CORRECT correct, WT_CHECK op) {
                // Build a hashmap: transition.src -> transitions
                typedef wali::HashMap<int, std::list<int> > trans_map_t;
                std::map<int, int> intra_node_map; // transition.src -> intra node number

                trans_map_t trans_map;
                set<int> states_visited;
                std::list<int> worklist;
                unsigned int i;
                IntraGraph *ca = new IntraGraph(true, sem); // running_prestar = true because extend goes backward
                //Transition initial_st(state, 0, 0);

                ca->setSource(get_number(intra_node_map,state,ca), sem->one());
                for(i=0;i<nodes.size();i++) {
                    trans_map_t::iterator it = trans_map.find(nodes[i].trans.src);
                    if(it == trans_map.end()) {
                        std::list<int> temp;
                        temp.push_back(i);
                        trans_map.insert(nodes[i].trans.src, temp);
                    } else {
                        it->second.push_back(i);
                    }
                    // add initial (state, stack, _) transitions
                    if(nodes[i].trans.src == state && nodes[i].trans.stack == stack) {
                        Transition t1(state,0,0);
                        Transition t2(nodes[i].trans.tgt,0,0);
                        ca->addEdge(get_number(intra_node_map,state,ca), get_number(intra_node_map, nodes[i].trans.tgt,ca), 
                                correct(nodes[i].gr->get_weight(nodes[i].intra_nodeno).get_ptr()));
                        worklist.push_back(nodes[i].trans.tgt);
                    }
                }
                states_visited.insert(state);
                while(!worklist.empty()) {
                    int st = worklist.front();
                    worklist.pop_front();
                    if(states_visited.find(st) != states_visited.end()) 
                        continue;
                    states_visited.insert(st);
                    trans_map_t::iterator trans_it = trans_map.find(st);
                    if(trans_it == trans_map.end())
                        continue;
                    std::list<int> &trans = trans_it->second;
                    std::list<int>::iterator it;
                    for(it = trans.begin(); it != trans.end(); it++) {
                        i = *it;
                        int t1 = get_number(intra_node_map,nodes[i].trans.src,ca);
                        int t2 = get_number(intra_node_map,nodes[i].trans.tgt,ca);
                        ca->addEdge(t1, t2, correct(nodes[i].gr->get_weight(nodes[i].intra_nodeno).get_ptr()));
                        if(states_visited.find(nodes[i].trans.tgt) == states_visited.end()) {
                            worklist.push_back(nodes[i].trans.tgt);
                        }
                    }
                }
                int final_st = get_number(intra_node_map,accept, ca);
                ca->setOutNode(final_st, 1); // second argument is not required
                ca->setupIntraSolution(false);
                bool r = op(ca->get_weight(final_st).get_ptr());
                delete ca;
                return r;
            }


    } // namespace graph

} // namespace wali

