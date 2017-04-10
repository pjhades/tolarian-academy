#Bayesian Network

* DAG
* Each node corresponds to a variable
* Acts as a filter which requires distributions to have certain conditional independence properties if they want to factorize w.r.t the graph
* Joint probability factorizes w.r.t. the conditionding nodes, i.e. parents
* Observed vs Latent variables
    * Latent variables allow us to build complex distributions upon simpler ones
    * Latent variables are often parameters
    * Observed variables are often data
    * Latent variables do not need to have physical interpretation
* Generative model
    * We can sample from it
    * It expresses the process by which the observed data arose
    * It encodes the causal process
* Two extremes
    * Fully-connected: no independent variables
    * No edge: joint = multiplication of marginals
    * Something in between: dropping edges to control model complexity
* Control the growth of parameter with the number of nodes
    * Alter the topology
    * Share parameters
    * Use parameterized model (e.g. Logistic vs N binary-valued)

##Conditional Independence

Three types of connection

* Tail-to-tail
    * `a <--- c ---> b`
    * conditional independent if C is **observed**
* Head-to-tail
    * `a ---> c ---> b`
    * conditional independent if C is **observed**
* Head-to-head
    * `a ---> c <--- b`
    * conditional independent if C and all C's descendants are **not observed**


##D-separation
Just the above types extended to set of nodes.

A path from A to B is blocked if either

* The path includes a Tail-to-tail or Head-to-tail node, and the node is conditioned on, or
* The path includes a Head-to-head node, but neither the node nor its descendants are conditioned on

If **all** paths from A to B are blocked, A and B are conditional independent given the conditioning set.

**Parameters** are treated as observed nodes, and have no marginal distribution, thus

* They must be tail-to-tail nodes on the paths through them
* They have no parent nodes
* They block paths through them

##Markov Blanket

Markov blanket of a node contains its

* Parents
* Children
* Co-parents

And nodes in the blanket are the only ones it depends on.
All other nodes are conditional independent with it.


#Markov Random Field

* undirected graph
* conditional independent if all paths are blocked by observed nodes
* Markov blanket is just the neighbors
* factorization is done w.r.t. cliques
* each clique has a **potential function**, and **Boltzmann distribution** if the exponential representation is used
* factorization is normalized by **partition function** Z
* the factorization
    * may have no probabilistic explanation
    * may not automatically be normalzied, thus needing explicit normalization with partition function
    * potential functions are **strictly positive**
* Hammersley-Clifford Theorem
    * connects undirected graph to factorization
    * a distribution factorizes according to the graph if it satisfies the conditional independence relations encoded by the graph
* convert DAGs to undirected ones
    * make sure nodes in a conditional probability reside in the same clique
    * by **marrying the parents**, called **moralization**
    * steps
        1. add undirected links between all pairs of parent nodes for each node
        2. drop directed arrows
