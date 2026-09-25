Fixed(1)),t.setAttribute("cy",(114.2705-.38*i*Math.cos(a)-8).toFixed(1)),t.setAttribute("r",(7.5*r*tA).toFixed(2)),t.setAttribute("opacity",((.3+.7*r)*tA).toFixed(3))}},ax=e=>{
let r=null==T.current?e:e0+T.current;
if(!G.current){
null!=Y.current&&(e7(tC(Math.round(Y.current),0,eK.length-1),10),l.x=1,l.v=0),C.x=1,C.v=0,c.x=1,c.v=0,c.t=1,u.x=1,u.v=0,u.t=1,g.x=0,g.v=0,g.t=0,x.x=0,x.v=0,x.t=0,ag(r),e3=r,e2=requestAnimationFrame(ax);
return}let s=Math.max(0,Math.min((r-e3)/1e3,.1));
e3=r,ta=s;
let y=tU[em.current]??null;
y!==U&&(U=y,F=r,L=!1);
let M=null!=y,w=em.current;
y&&to.has(w)&&(!L&&r-F>(tA[w]??2500)?(L=!0,S=r):L&&r-S>1500&&(L=!1,F=r),M=!L),E.t=+!!M,y&&y!==I&&(I&&E.x>.02?(B=I,N.x=0,N.v=0,N.t=1,i&&(N.x=1)):(B=null,N.x=1,N.v=0,N.t=1),I=y,ev=r),!y&&E.x<.004&&(I=null,B=null),N.x>.996&&(B=null),M!==ew&&(M&&!i&&(eM=.5>t()?1:-1),i||(ed.t=ey+=ej*eM),ew=M),i?(e7(th[em.current][0]),d.t=0,o.t=0,A.t=0,h.t=1,c.t=1,u.t=1):((e=>{
let i=em.current,r=(e-e0)/1e3,s=(e-eC.current)/1e3;
if(ek!==i&&(ek=i,eF=0,eE=e+a(...tc[i]),eN=e+a(1500,7e3),eS=e+("excited"===i?a(400,1100):"searching"===i?a(800,1600):"working"===i?a(1200,2400):a(6e3,1e4)),eV=e+a(500,1200),eB=e+a(1200,2200),eU=0,p=e+a(500,1400),f=e+a(3e3,8e3),eD=-1,eW=-1,eO=[],tN=!1,"celebrate"===i&&(eR=e+140),"waking"!==i&&"sleeping"!==i&&("drowsy"!==i&&t8(e),e7(th[i][0],"excited"===i?10:8))),"celebrate"===i&&!t2&&e>=eR&&(t4("spinWild"),eR=e+6200),e>=tj){
let r=tg.has(i),s=tx.has(i);
if((r||s)&&!e9&&tZ<0&&!t2){
let e=t();
r?e<.55?tp(1):t4("spinBounce"):e<.34?t4("spinBounce"):e<.62?t3():e<.86?t4("spinDizzy"):tp(1)}tj=e+a(9e3,18e3)}let C=1,y=1;
switch(i){
case"sleeping":{
let e=th.sleeping.includes(n);
ei.current?(e||e7(13,5.8),C=1):e?C=l.x>.85?1:.08:s<1.2?C=Math.max(.08,1-Math.min(1,s/1)*(1+.15*Math.sin(6.5*s))):(C=.08,c.x<.18&&e7(13,11));
let t=Math.min(s/2,1),a=Math.sin(tC(s/.5,0,1)*Math.PI);
d.t=0+4*t+2*Math.sin(.25*r),o.t=-2*t,A.t=8*t+3*Math.sin(.55*r)-5*a,h.t=1+.016*Math.sin(.55*r)+.05*a;
break}case"waking":if(ei.current){
if(s<.75)e7(3,6.6),C=1,y=1+.08*tC(s/.6,0,1),A.t=4-10*tC(s/.75,0,1),o.t=0,d.t=0,h.t=1.02,s>.2&&!tN&&(tI.burst(a(9,13),.8),tN=!0);
else if(s<1.45)e7(0,7),C=1,A.t=0,h.t=1;
else{
let e=Math.min((s-1.45)/.65,1);
e7(0,7.2),d.t=0+6*Math.sin(e*Math.PI*3)*(1-e),A.t=2*Math.sin(.9*r)}break}if(s<.5)C=.07,e7(3,12),A.t=6;
else if(s<1.2)C=1,y=1.12,A.t=-5,o.t=0,d.t=0,h.t=1.04,tN||(tI.burst(a(9,13),.8),tN=!0);
else if(s<2.2)0===eL.length&&s<1.4&&t8(e),e7(0),A.t=0,h.t=1;
else{
let e=Math.min((s-2.2)/.8,1);
e7(0),d.t=0+6*Math.sin(e*Math.PI*3)*(1-e),A.t=2*Math.sin(.9*r)}break;
case"idle":d.t=0+1.5*Math.sin(.5*r)+.6*Math.sin(.17*r),o.t=+Math.sin(.27*r),A.t=1.2*Math.sin(.85*r),h.t=1+.007*Math.sin(.85*r);
break;
case"listening":if(d.t=8+1.5*Math.sin(.5*r),o.t=2,A.t=-2+.8*Math.sin(.8*r),h.t=1.015,e>=eB&&(eI=e+380,eB=e+a(1800,3200)),e<eI){
let t=1-(eI-e)/380;
A.t+=4.5*Math.sin(t*Math.PI),d.t+=2*Math.sin(t*Math.PI)}break;
case"thinking":d.t=-9+5*Math.sin(.35*r),o.t=5*Math.sin(.3*r),A.t=2.5*Math.sin(.6*r),h.t=1;
break;
case"searching":{
let t=Math.sin(1.3*r);
d.t=0+13*t,o.t=7*t,A.t=3*Math.sin(1.7*r),h.t=1,e>=eS&&(tp(),eS=e+a(4e3,7e3));
break}case"working":{
let t=Math.sin(r*Math.PI*3.2);
d.t=4+2.5*t,o.t=3,A.t=1.5+3*Math.max(0,t),h.t=1-.02*Math.max(0,t),e>=eS&&(tp(1,1),eS=e+a(6e3,9e3));
break}case"excited":{
let t=2.2*r%1;
A.t=-(10*Math.sin(t*Math.PI))+2,h.t=t<.1?.92:t<.3?1.05:1,o.t=4*Math.sin(1.1*r),y=1.06,e>=eS&&(tp(1),eS=e+a(2800,5e3)),d.t=0+7*Math.sin(r*Math.PI*2.2);
break}case"surprised":{
let e=Math.min(s/1.2,1);
o.t=-4*(1-e),A.t=-8*(1-e),h.t=s<.2?1.08:1,y=1.15-.08*e,d.t=0+1.5*Math.sin(11*r)*(1-e);
break}case"suspicious":d.t=-6+3*Math.sin(.3*r),o.t=-4*Math.sin(.25*r),A.t=1+1.2*Math.sin(.45*r),h.t=1,C=.85,e>=eV&&(d.v+=30,eV=e+a(4e3,7e3));
break;
case"angry":e>=eV&&(e1=e+420,A.v+=70,eV=e+a(1800,3200)),d.t=0+(e<e1?4.5*Math.sin(.05*e):0),o.t=0,A.t=3.5,h.t=.975;
break;
case"drowsy":if(d.t=0+2.5*Math.sin(.32*r),o.t=1.5*Math.sin(.2*r),A.t=6+2.2*Math.sin(.36*r),h.t=1+.022*Math.sin(.36*r),C=.34+.07*Math.sin(.8*r),e>=eB&&!eU&&(eU=e),eU){
let t=(e-eU)/1e3;
if(t<1.7){
let e=t/1.7,a=e*e;
A.t=6+19*a+2.2*Math.sin(e*Math.PI*2.5)*(1-e),d.t=0+10*a,C=.34-a*(.34-.04),h.t=1-.045*a}else if(t<2){
let e=Math.sin((t-1.7)/.3*Math.PI);
A.t=25-7*e,d.t=10-4*e,C=.04+.42*e}else if(t<3.5){
let e=(t-1.7-.3)/1.5,a=1-Math.pow(1-e,2.2);
A.t=25+-19*a,d.t=0+10*(1-a),C=.46+-.12*a,e>.32&&e<.46&&(C=.05)}else eU=0,eB=e+a(1500,3500)}break;
case"happy":{
let e=Math.sin(2.4*r);
d.t=0+3*Math.sin(1.2*r),o.t=2.5*Math.sin(1.1*r),A.t=-(3*Math.abs(e)),h.t=1+.02*e,y=1.05;
break}case"curious":if(d.t=10+6*Math.sin(.7*r),o.t=5*Math.sin(.6*r),A.t=-2+1.5*Math.sin(.9*r),h.t=1.01,y=1.08,e>=eB&&(eI=e+440,eB=e+a(1600,2800)),e<eI){
let t=1-(eI-e)/440;
o.t+=8*Math.sin(t*Math.PI),d.t+=5*Math.sin(t*Math.PI)}break;
case"confused":{
let t=Math.sin(.8*r);
d.t=0+12*t,o.t=3*t,A.t=2*Math.sin(.5*r),h.t=1,C=.9,e>=eV&&(d.v+=22,eV=e+a(2600,4200));
break}case"bored":if(d.t=-3+4*Math.sin(.25*r),o.t=4*Math.sin(.2*r),A.t=5+1.5*Math.sin(.35*r),h.t=.99,C=.6,y=.98,e>=eV&&(eI=e+600,eV=e+a(4e3,7e3)),e<eI){
let t=1-(eI-e)/600;
h.t=1+.05*Math.sin(t*Math.PI),A.t+=3*Math.sin(t*Math.PI)}break;
case"proud":d.t=0+2.5*Math.sin(.4*r),o.t=2*Math.sin(.35*r),A.t=-4+Math.sin(.6*r),h.t=1.03,y=1.02,C=.9;
break;
case"shy":d.t=-8+3*Math.sin(.5*r),o.t=-3+2*Math.sin(.4*r),A.t=3,h.t=.98,y=.95,C=.85;
break;
case"sad":d.t=3+2*Math.sin(.3*r),o.t=1.5*Math.sin(.25*r),A.t=7+Math.sin(.4*r),h.t=.97,C=.7,y=.97;
break;
case"laughing":{
let e=Math.sin(r*Math.PI*6.4);
d.t=0+4*e,o.t=2*Math.sin(2*r),A.t=-(5*Math.abs(e)),h.t=1+.03*e,C=.7,y=1;
break}case"scared":d.t=0+2*Math.sin(.04*e),o.t=-2+1.5*Math.sin(.05*e),A.t=2+Math.sin(1.5*r),h.t=.97,y=1.12,C=1.05;
break;
case"playful":d.t=0+8*Math.sin(1.4*r),o.t=4*Math.sin(1.1*r),A.t=-(3*Math.abs(Math.sin(2.2*r))),h.t=1+.015*Math.sin(2.2*r),y=1.06,e>=eS&&(tp(1),eS=e+a(3500,6e3));
break;
case"celebrate":d.t=0,o.t=0,A.t=-(2.5*Math.abs(Math.sin(1.6*r))),h.t=1,y=1.1,C=1.1;
break;
case"orbit":case"radar":case"progress":case"spawning":case"loading":case"dictating":case"sending":case"receiving":case"uploading":case"writing":case"alerting":case"bouncing":case"powering-down":d.t=0,o.t=0,A.t=0,h.t=1;
break;
case"dragging":{
let e=s%3.4/3.4,t=Math.floor(s/3.4);
e<.12?(o.t=-16,A.t=-22,d.t=-5):e<.62?(o.t=-16+32*ty((e-.12)/.5),A.t=-22+2*Math.sin(1.4*r),d.t=0+6*Math.sin(2.6*r),y=1.06):(t!==eD&&(eD=t,A.v+=90),o.t=16,A.t=0,d.t=0),h.t=1;
break}case"humming":d.t=0+2*Math.sin(.4*r),o.t=1.5*Math.sin(.3*r),A.t=1.5*Math.sin(.7*r),h.t=1;
break;
case"notifying":eD<0&&s>.12&&(eD=0,A.v-=26,t8(e)),y=1+.05*Math.exp(-(3*s)),d.t=3,o.t=2,A.t=-1,h.t=1}if(e>=p){
let r=()=>.5>t()?-1:1,s=0,l=0,n=2500,d=5e3;
switch(i){
case"idle":s=0,l=0,n=2500,d=5500;
break;
case"listening":s=15*a(-.3,.3),l=9*a(-.25,.25),n=2200,d=4200;
break;
case"thinking":s=r()*a(.5,1)*15,l=-(9*a(.4,1)),n=1500,d=2800;
break;
case"searching":s=r()*a(.7,1)*15,l=9*a(-1,1),n=550,d=1150;
break;
case"working":s=15*a(-.4,.4),l=9*a(.4,1),n=1200,d=2400;
break;
case"excited":s=15*a(-1,1),l=9*a(-1,.3),n=700,d=1400;
break;
case"surprised":s=0,l=0,n=1600,d=2600;
break;
case"suspicious":s=15*r(),l=2.6999999999999997,n=2200,d=4200;
break;
case"angry":s=15*a(-.2,.2),l=1.8,n=1800,d=3200;
break;
case"drowsy":s=15*a(-.4,.4),l=9*a(.4,1),n=2500,d=4500;
break;
case"happy":s=15*a(-.7,.7),l=-(9*a(0,.6)),n=1800,d=3400;
break;
case"curious":s=r()*a(.6,1)*15,l=9*a(-1,1),n=950,d=1900;
break;
case"confused":s=r()*a(.5,1)*15,l=9*a(-.6,1),n=1100,d=2300;
break;
case"bored":s=r()*a(.7,1)*15,l=9*a(.4,.9),n=3e3,d=6e3;
break;
case"proud":s=15*a(-.3,.3),l=-(9*a(.3,.7)),n=2600,d=4600;
break;
case"shy":s=r()*a(.6,1)*15,l=9*a(.5,1),n=2e3,d=4e3;
break;
case"sad":s=15*a(-.3,.3),l=9*a(.6,1),n=2800,d=5e3;
break;
case"laughing":s=15*a(-.5,.5),l=-(9*a(.2,.6)),n=800,d=1700;
break;
case"scared":s=r()*a(.7,1)*15,l=9*a(-.6,.6),n=450,d=1050;
break;
case"playful":s=r()*a(.5,1)*15,l=-(9*a(0,.6)),n=900,d=1800;
break;
case"notifying":{
let e=.72>t();
s=(e?.45:.1)*15,l=-(9*(e?.3:.05)),n=1200,d=2400;
break}default:s=15*a(-.4,.4),l=9*a(-.3,.3)}g.t=s,x.t=l,p=e+a(n,d)}if(("idle"===i||"happy"===i||"excited"===i||"curious"===i||"playful"===i)&&e>=f&&(b=e,m=.5>t()?0:1,f=e+a(4500,1e4)),t6=null,tq=0,tX=0,t$=0,t_=0,t1=0,t0=0,t2){
let t=(e-t2.t0)/1e3,{
kind:a,dir:i,turns:r}=t2;
if("spinDizzy"===a){
let e=.55+.16*r;
if(t<e){
let a=t/e;
t6=r*Math.PI*2*i*(a*a)}else if(t<e+1.5){
let a=t-e,r=Math.pow(1-a/1.5,1.3);
tq=17*Math.sin(10*a)*i*r,tX=10*Math.cos(10*a)*i*r,t$=3*Math.sin(20*a)*r,C=.46+.14*Math.sin(21*a),y=1.03}else t2=null}else if("spinWild"===a){
let e=2.3-.3,a=2*Math.PI,s=(r*a+.5)/(.15+(2.3-.3)+.3125);
if(t<5.49){
let l;
if(t<.24)l=-.5*(1-Math.cos(t/.24*Math.PI))/2;
else if(t<.54){
let e=t-.24;
l=-.5+s*e*e/.6}else l=t<2.54?-.5+s*(.15+(t-.24-.3)):t<3.79?-.5+s*(.15+e)+1.25*s*(1-Math.pow(1-(t-.24-2.3)/1.25,4))/4:r*a;
t6=l*i;
let n=0;
if(t>2.54){
let e=Math.min((t-.24-2.3)/1.25,1);
n=e<.4?0:Math.pow((e-.4)/.6,2),t>=3.79&&(n=Math.pow(1-(t-3.79)/1.7,1.6))}let d=Math.max(t-.24-2.3,0);
t_=l/(r*a)*1080*i,tq=11*Math.sin(9.2*d)*i*n,tX=(Math.cos(9.2*d)-1)*6*i*n,t$=2.6*Math.sin(18.4*d)*n,t1=13*Math.sin(11.5*d)*i*n,t0=(Math.cos(9*d)-1)*3.5*n,C=1.14-.44*n+.1*Math.sin(16*d)*n,y=1.12-.09*n}else t2=null}else"spinBounce"===a&&(t<.7?t6=r*Math.PI*2*i*ty(t/.7):(t3(),t2=null))}if(tJ=0,tZ>=0){
let t=(e-tZ)/1e3;
if(t>=tY)tZ=-1;
else{
let e=0,a=0;
for(;
a<tO.length&&!(t<e+tO[a].d);
a++)e+=tO[a].d;
let{
h:i,d:r}=tO[a],s=(t-e)/r;
tJ=-4*i*s*(1-s)}}if("waking"!==i&&"sleeping"!==i&&e>=eE){
let t=th[i];
eF=(eF+1+Math.floor(a(0,t.length-1)))%t.length,e7(t[eF],"searching"===i||"excited"===i?10:6),eE=e+a(...tc[i])}let M=tu[i];
M&&e>=eN&&(t8(e),eN=e+a(M[0],M[1]));
let w=null;
for(;
eL.length&&e>=eL[0].at;
)w=eL[0].v,eL.shift();
null!=ea.current&&(C=Math.max(C,ea.current)),"working"===i&&(y*=1+.14*tv(tC((-g.x-.5)/4.5,0,1))*tv(tC((x.x-3)/6,0,1))),c.t=w??(eL.length?c.t:C),u.t=y})(r),null!=Y.current&&e7(tC(Math.round(Y.current),0,eK.length-1),10)),W.current&&(u.t=Math.max(u.t,1.32),c.t=Math.max(c.t,1.18));
let v=Math.max(1,Math.ceil(s/tm)),j=s/v;
for(let e=0;
e<v;
e++)tb(l,e8,1,j),e9&&tb(e9,6.2,1,j),tb(d,5,.9,j),tb(o,3.5,1,j),tb(A,4,1,j),tb(h,10,.8,j),tb(c,26,1,j),tb(u,9,.85,j),tb(eP,9,.55,j),tb(eQ,6,1,j),tb(g,13,1,j),tb(x,13,1,j),tb(E,14,1,j),tb(N,11,1,j),tb(C,10,1,j),tb(ed,14,1,j);
i&&(N.x=1,ed.x=ed.t,E.x=E.t),ag(r),(e=>{
if(e-tn<500||!D.current)return;
tn=e;
let t=D.current.getBoundingClientRect().width;
t>0&&(ti=tC((340/t)**.7,1,1+1.6*es.current.smallBoost),tr=t)})(r);
let k="humming"===em.current,R="loading"===em.current,V=el.current;
if(eQ.t=+!!k,(k||R||V)&&!i){
let e=(r-eC.current)/1e3,t=R?3:1.6,a=V?7:e<.5?7*ty(e/.5):e<1.3?7+(t-7)*ty((e-.5)/.8):t+.3*Math.sin(.5*e);
eH+=a*s}e9?tL=e9.x:null!==t6?tL=t6:(k||R||V)&&(tL=eH),tI.update(r,s,{
spinAngle:tL,sizeScale:ti,wideStyle:t2?.kind==="spinWild"||t7||k||V}),e2=requestAnimationFrame(ax)};
return z.current={
spin:(e=1)=>tp(e),bounce:()=>t3(),burst:()=>tI.burst(22,1.1,.3),snapshot:()=>{
let e=D.current;
if(!e)return"";
let t=e.cloneNode(!0),a=[e,...e.querySelectorAll("*")],i=[t,...t.querySelectorAll("*")];
return a.forEach((e,t)=>{
let a=i[t];
if(!a)return;
let r=getComputedStyle(e);
for(let e of["fill","stroke","stroke-width","stroke-linecap","stroke-linejoin","opacity","display"]){
let t=r.getPropertyValue(e);
t&&a.style.setProperty(e,t)}}),t.outerHTML}},e2=requestAnimationFrame(ax),()=>cancelAnimationFrame(e2)},[w]);
let ey=eA(t);
return(0,b.jsxs)("svg",{
ref:D,className:i?`${
tN.default.svg} ${
i}`:tN.default.svg,style:{
overflow:"visible",...a?{
width:a,height:a}:{
},"--fg":j,"--bg":k,transform:h?`scale(${
A.scale})`:`perspective(720px) rotateX(${
A.tilt}deg) rotateY(${
A.turn}deg) rotateZ(${
A.roll}deg) scale(${
A.scale})`,transformOrigin:"50% 50%"},"data-state":e,viewBox:"-15 -15 259 259",xmlns:"http://www.w3.org/2000/svg",children:[(0,b.jsxs)("defs",{
children:[(0,b.jsx)("clipPath",{
id:V,children:(0,b.jsx)("path",{
ref:ec,d:ey.path})}),R&&(L=Math.cos(F=(R.angle??90)%360*Math.PI/180)/2,S=Math.sin(F)/2,(0,b.jsxs)("linearGradient",{
id:`${
V}-ink`,x1:.5-L,y1:.5-S,x2:.5+L,y2:.5+S,children:[(0,b.jsx)("stop",{
offset:R.fromPos??0,stopColor:R.from}),(0,b.jsx)("stop",{
offset:Math.max(R.toPos??1,R.fromPos??0),stopColor:R.to})]}))]}),(0,b.jsx)("g",{
ref:Q,"aria-hidden":"true"}),[0,1].map(e=>(0,b.jsx)("path",{
className:tN.default.head,d:tF,style:{
display:"none"},ref:t=>{
eg.current[e]=t}},e)),[0,1,2,3,4].map(e=>(0,b.jsx)("circle",{
cx:114.2705,cy:114.2705,r:0,fill:"none",style:{
display:"none",stroke:"var(--fg)"},ref:t=>{
eb.current[e]=t}},`ring${
e}`)),[0,1,2,3,4].map(e=>(0,b.jsx)("circle",{
className:tN.default.head,cx:114.2705,cy:114.2705,r:0,style:{
display:"none"},ref:t=>{
ef.current[e]=t}},`part${
e}`)),[0,1,2].map(e=>(0,b.jsx)("path",{
style:{
display:"none"},ref:t=>{
ep.current[e]=t}},`glyph${
e}`)),(0,b.jsxs)("g",{
ref:H,children:[(0,b.jsx)("path",{
ref:eh,className:tN.default.head,d:ey.path,style:R?{
fill:`url(#${
V}-ink)`}:void 0}),(0,b.jsxs)("g",{
clipPath:`url(#${
V})`,children:[(0,b.jsx)("path",{
className:tN.default.eye,ref:e=>{
eu.current[0]=e}}),(0,b.jsx)("path",{
className:tN.default.eye,ref:e=>{
eu.current[1]=e}})]}),(0,b.jsx)("circle",{
ref:ex,cx:114.2705,cy:114.2705,r:0,style:{
display:"none",stroke:"var(--bg)",strokeWidth:6}})]}),(0,b.jsx)("g",{
ref:P,"aria-hidden":"true"})]})});
try{
var tK=window;
tK._sentryModuleMetadata=tK._sentryModuleMetadata||{
},tK._sentryModuleMetadata[(new tK.Error).stack]=Object.assign({
},tK._sentryModuleMetadata[(new tK.Error).stack],{
"_sentryBundlerPluginAppKey:website":!0})}catch(e){
}try{
var tz=window;
tz._sentryModuleMetadata=tz._sentryModuleMetadata||{
},tz._sentryModuleMetadata[(new tz.Error).stack]=Object.assign({
},tz._sentryModuleMetadata[(new tz.Error).stack],{
"_sentryBundlerPluginAppKey:website":!0})}catch(e){
}let tG={
size:1,gap:1,height:1,eyeWidth:1.2,eyeHeight:1.16},tT={
size:.86,gap:1.18,height:1,eyeWidth:.96,eyeHeight:.92},tY={
angry:{
gap:1.28,size:.78,eyeWidth:.88,eyeHeight:.84},suspicious:{
gap:1.24,size:.82,eyeWidth:.9},confused:{
gap:1.2,size:.84,eyeWidth:.9},scared:{
size:.8,eyeWidth:.9,eyeHeight:.88},surprised:{
size:.76,eyeWidth:.86,eyeHeight:.86},excited:{
size:.78,eyeWidth:.88,eyeHeight:.88},celebrate:{
size:.74,eyeWidth:.84,eyeHeight:.84},happy:{
size:.76,eyeWidth:.86,eyeHeight:.84},curious:{
size:.84},drowsy:{
size:.92,eyeWidth:.96},bored:{
size:.92,eyeWidth:.96},sad:{
size:.92,eyeWidth:.96},playful:{
size:.84,gap:1.2}},tZ=new Set(["idle","working"]);
function tJ(e){
let t=tY[e];
return{
...tT,...t,gap:Math.max(t?.gap??tT.gap,1.14),size:Math.min(t?.size??tT.size,.92),eyeWidth:Math.min(t?.eyeWidth??tT.eyeWidth,1),eyeHeight:Math.min(t?.eyeHeight??tT.eyeHeight,1),eyeWidthRight:t?.eyeWidthRight,eyeHeightRight:t?.eyeHeightRight}}function tq(e,t,a){
let i=t.eyeWidthRight??t.eyeWidth,r=t.eyeHei