// Annotated version of
// https://code.jsoftware.com/wiki/Essays/Incunabulum

typedef char C;typedef long I;

// This defines the general data type. The members are
// t: mark if the data is boxed
// r: rank, so it's also the number of dimension in d
// d: shape
// p: actual content
typedef struct a{I t,r,d[3],p[2];}*A;

#define P printf
#define R return

// V1 defines functions like f: A -> A
#define V1(f) A f(w)A w;
// V2 defines functions like f: A, A -> A
#define V2(f) A f(a,w)A a,w;

// Repeat opeation x n times. x will possibly use variable i.
#define DO(n,x) {I i=0,_n=(n);for(;i<_n;++i){x;}}

// Allocate a long[n] array. 
I *ma(n){R(I*)malloc(n*4);}

// Copy n items from source s to destination d
mv(d,s,n)I *d,*s;{DO(n,d[i]=s[i]);}

// Compute the total number of elements given the shape
tr(r,d)I *d;{I z=1;DO(r,z=z*d[i]);R z;}

// This function allocates a new object of rank r and shape d.
// The 5+tr(r,d) probably guarantees that at least each object
// will contain t, r and d[3] fields.
A ga(t,r,d)I *d;{A z=(A)ma(5+tr(r,d));z->t=t,z->r=r,mv(z->d,d,r);
 R z;}

// Below are functions supported by this simple J interpreter

// Iota function i.5 returns 0 1 2 3 4
V1(iota){I n=*w->p;A z=ga(0,1,&n);DO(n,z->p[i]=i);R z;}

// Add two operands
V2(plus){I r=w->r,*d=w->d,n=tr(r,d);A z=ga(0,r,d);
 DO(n,z->p[i]=a->p[i]+w->p[i]);R z;}

// x from y, fetch the x-th element of y
// The key to infer this is the w->p + (n**a->p)
// as it skips n rows. *a->p is the length of the highest dimension
V2(from){I r=w->r-1,*d=w->d+1,n=tr(r,d);
 A z=ga(w->t,r,d);mv(z->p,w->p+(n**a->p),n);R z;}

// Box the operand, convert it into a pointer to long
V1(box){A z=ga(1,0,0);*z->p=(I)w;R z;}

// Concatenate the content of two operands
V2(cat){I an=tr(a->r,a->d),wn=tr(w->r,w->d),n=an+wn;
 A z=ga(w->t,1,&n);mv(z->p,a->p,an);mv(z->p+an,w->p,wn);R z;}

// He had something unfinished that afternoon
V2(find){}

// x rsh y creates an array of shape x, then initialize
// the array with elements from y. If there's not enough
// elements in y, reuse the elements of y from the beginning
V2(rsh){I r=a->r?*a->d:1,n=tr(r,a->p),wn=tr(w->r,w->d);
 A z=ga(w->t,r,a->p);mv(z->p,w->p,wn=n>wn?wn:n);
 if(n-=wn)mv(z->p+wn,z->p,n);R z;}

// Return the shape of the operand
V1(sha){A z=ga(0,1,&w->r);mv(z->p,w->d,w->r);R z;}

// Identity function
V1(id){R w;}

// Return length of the highest dimension
V1(size){A z=ga(0,0,0);*z->p=w->r?*w->d:1;R z;}

pi(i){P("%d ",i);}
nl(){P("\n");}

// Pretty print an object. It's called recursively if the
// argument is boxed
pr(w)A w;{I r=w->r,*d=w->d,n=tr(r,d);DO(r,pi(d[i]));nl();
 if(w->t)DO(n,P("< ");pr(w->p[i]))else DO(n,pi(w->p[i]));nl();}

// The alphabet, all vailable verbs
C vt[]="+{~<#,";

// Maps the index of a verb in vt[] to the corresponding
// function pointer. vd: dyadic case, vm: monadic case
A(*vd[])()={0,plus,from,find,0,rsh,cat},
 (*vm[])()={0,id,size,iota,box,sha,0};

// This stores the variable's values when we see assignment
I st[26];

qp(a){R  a>='a'&&a<='z';}
qv(a){R a<'a';}

// Execute the program
// 4 cases are handled here: assignment, variable reference, monad, dyad
A ex(e)I *e;{I a=*e;
 if(qp(a)){if(e[1]=='=')R st[a-'a']=ex(e+2);a= st[ a-'a'];}
 R qv(a)?(*vm[a])(ex(e+1)):e[1]?(*vd[e[1]])(a,ex(e+2)):(A)a;}

// Construct an object for a noun
noun(c){A z;if(c<'0'||c>'9')R 0;z=ga(0,0,0);*z->p=c-'0';R z;}

// Find the index of a verb in the alphabet
verb(c){I i=0;for(;vt[i];)if(vt[i++]==c)R i;R 0;}

// Parsing, convert input program into nouns or verbs
I *wd(s)C *s;{I a,n=strlen(s),*e=ma(n+1);C c;
 DO(n,e[i]=(a=noun(c=s[i]))?a:(a=verb(c))?a:c);e[n]=0;R e;}

// REPL
main(){C s[99];while(gets(s))pr(ex(wd(s)));}
