// Real Joey photo; the drawn cat is the fallback if this is emptied.
const JOEY_PHOTO="/life/readme/joey.jpg";
const JOEY_SVG=`<svg viewBox="0 0 64 64" role="img" aria-label="Joey the cat"><circle cx="32" cy="32" r="31" fill="var(--bg)" stroke="var(--line)"/><path d="M14 22 L18 6 L28 16 Z M50 22 L46 6 L36 16 Z" fill="#e09a4f"/><ellipse cx="32" cy="35" rx="20" ry="17" fill="#e09a4f"/><path d="M24 22 q8 -4 16 0" stroke="#b86f2c" stroke-width="2" fill="none"/><ellipse cx="25" cy="33" rx="3" ry="4" fill="#1c1b19"/><ellipse cx="39" cy="33" rx="3" ry="4" fill="#1c1b19"/><circle cx="26" cy="32" r="1" fill="#fff"/><circle cx="40" cy="32" r="1" fill="#fff"/><path d="M30 39 L34 39 L32 41 Z" fill="#d9636b"/><path d="M32 41 q-3 4 -6 2 M32 41 q3 4 6 2" stroke="#1c1b19" stroke-width="1.2" fill="none"/><path d="M12 38 L22 39 M12 42 L22 41 M52 38 L42 39 M52 42 L42 41" stroke="#1c1b19" stroke-width=".8"/></svg>`;
const JOEY={
 basics:"Hi, I'm Joey. Start easy: who are you? I already know who I am.",
 first24:"If you're stuck in hospital, someone has to feed me by 6pm. Who gets the first call?",
 people:"Pick your people. I'll make sure each one hears from you — gently.",
 health:"Doctors ask a lot of questions. Answer them here once, so nobody guesses for you.",
 legal:"I'm a cat, not a lawyer. Tell me where the real will is, or if there isn't one yet.",
 money:"Names of banks only. No numbers. I can't count anyway.",
 digital:"Where's the key to the keys? Say where your password manager's emergency kit lives.",
 home:"The house, the car, the plants nobody waters. And the pets. Especially the pets.",
 wishes:"Last one. The part people wish they'd asked about. Take your time — I'll nap."
};
const JOEY_SECRET="Psst. That looks like a password or account number. Say where it's kept instead — people will read this.";
const JOEY_DONE="All nine lives done. Download it, put a reminder in your calendar, and tell your people. I'll be on the windowsill.";
const S=[
 {id:"basics",t:"Basics",why:"Who this is about, and when it was last true.",f:[
  ["name","Full name"],["dob","Date of birth","date"],["location","City / country"],
  ["langs","Languages, faith, anything a stranger caring for you should know","area"]]},
 {id:"first24",t:"First 24 hours",why:"The first things someone should do if you can't speak for yourself.",f:[
  ["call_first","Call these people first, in this order","area","Partner — +1 555…\nSister Ana — +1 555…"],
  ["urgent","Urgent things that will break without you","area","Kids pickup at 3pm, dog needs meds twice daily, payroll runs Fridays"],
  ["keys","Where keys, phone PIN hint, and the 'emergency folder' are","area"]]},
 {id:"people",t:"People",why:"Who does what. They'll each get a heads-up that you named them.",f:[
  ["executor","Executor / person in charge"],["backup","Backup if they can't"],
  ["guardian","Guardian for kids / dependants"],["pet_guardian","Who takes the pets"],
  ["dont","Anyone who should NOT be involved (optional)","area"]]},
 {id:"health",t:"Health & care",why:"What a doctor or your family needs on day one.",f:[
  ["conditions","Conditions, meds, allergies","area"],["doctor","GP / main doctor"],
  ["directive","Advance directive / health proxy: exists? where?"],
  ["donor","Organ donation",["Yes","No","Ask my family","Not decided"]],
  ["care","Care wishes (resuscitation, life support, where you'd want to be)","area"]]},
 {id:"legal",t:"Will & legal",why:"The readme is not a will. This says where the real documents are.",f:[
  ["will","Do you have a will?",["Yes — signed","Drafted, not signed","No","Not sure"]],
  ["will_where","Where the original is"],["lawyer","Lawyer / notary contact"],
  ["insurance","Life insurance: provider + where the policy is"],["poa","Power of attorney: who, where"]]},
 {id:"money",t:"Money map",why:"Where things are, never the numbers. Banks, pensions, debts, who to notify.",f:[
  ["banks","Banks & institutions (names only)","area","Chase checking, Fidelity 401k, Amex card"],
  ["bills","Bills that auto-pay and from where","area"],["debts","Loans / mortgage / who's owed money","area"],
  ["accountant","Accountant / financial adviser"]]},
 {id:"digital",t:"Digital",why:"Where the keys are, not the keys.",f:[
  ["pwm","Password manager + where its emergency kit is","text","1Password — emergency kit in the fire safe"],
  ["phone","Phone & laptop: how should someone get in?","area"],
  ["legacy","Legacy contacts set? (Apple, Google, Facebook…)","area"],
  ["accounts","Accounts to memorialise / delete / keep running","area"]]},
 {id:"home",t:"Home & pets",why:"The physical stuff that quietly falls apart.",f:[
  ["home","Rent / own, landlord or mortgage contact"],["utilities","Utilities & who to call","area"],
  ["pets","Pets: names, vet, food, meds, quirks","area"],["car","Vehicles: where, papers, keys"]]},
 {id:"wishes",t:"Wishes",why:"Optional. The part people regret not knowing.",f:[
  ["funeral","Funeral / ceremony / burial or cremation","area"],
  ["music","Music, readings, who speaks","area"],["belongings","Things that should go to someone specific","area"],
  ["notes","Notes to specific people","area"]]}
];
const KEY="aedae.life-readme.draft.v0";
let data={},cur=0;
try{data=JSON.parse(localStorage.getItem(KEY)||"{}")}catch(e){data={}}
const $=s=>document.querySelector(s);
const esc=s=>String(s).replace(/[&<>"]/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));
const SECRET=/(password|passcode|pin)\s*[:=]\s*\S|\b\d{12,19}\b|\b\d{3}-\d{2}-\d{4}\b|seed phrase/i;

function save(){try{localStorage.setItem(KEY,JSON.stringify(data))}catch(e){}}
function filled(sec){return sec.f.some(([k])=>(data[k]||"").trim())}

function tabs(){
 $("#tabs").innerHTML=S.map((s,i)=>`<button data-i="${i}" aria-current="${i===cur}" class="${filled(s)?"done":""}">${s.t}</button>`).join("");
}
function form(){
 const s=S[cur];
 const face=JOEY_PHOTO?`<img src="${JOEY_PHOTO}" alt="Joey the cat">`:JOEY_SVG;
 $("#form").innerHTML=`<div class="joey">${face}<p class="bubble" id="joeysays" aria-live="polite"><b>Joey</b><span>${JOEY[s.id]}</span></p></div><h2>${s.t}</h2><p class="why">${s.why}</p>`+
  s.f.map(([k,l,type,ph])=>{
   const v=esc(data[k]||""),p=ph?` placeholder="${esc(ph)}"`:"";
   let c;
   if(Array.isArray(type))c=`<select id="${k}"><option value=""></option>${type.map(o=>`<option${data[k]===o?" selected":""}>${esc(o)}</option>`).join("")}</select>`;
   else if(type==="area")c=`<textarea id="${k}"${p}>${v}</textarea>`;
   else c=`<input id="${k}" type="${type==="date"?"date":"text"}" value="${v}"${p}>`;
   return `<label for="${k}">${l}</label>${c}`;
  }).join("")+
  `<div class="warn" id="secret" hidden>Joey flagged this: it looks like a password, PIN or account number.</div>
   <div class="row"><button class="btn ghost" id="prev"${cur?"":" disabled"}>Back</button>
   <button class="btn" id="next">${cur<S.length-1?"Next: "+S[cur+1].t:"Done — download below"}</button></div>`;
}
function md(){
 const today=new Date().toISOString().slice(0,10);
 let o=`---\nformat: life-readme/0.1\nname: ${JSON.stringify(data.name||"")}\nupdated: ${today}\nreview: yearly\n---\n\n# Life README — ${data.name||"(your name)"}\n\n> Not a will. This file says where things are and what I want. Last updated ${today}.\n`;
 for(const s of S){
  const rows=s.f.filter(([k])=>(data[k]||"").trim());
  if(!rows.length)continue;
  o+=`\n## ${s.t}\n`;
  for(const [k,l] of rows){
   const v=data[k].trim();
   o+=v.includes("\n")?`\n**${l}**\n\n${v.split("\n").map(x=>"- "+x.replace(/^[-*]\s*/,"")).join("\n")}\n`:`\n**${l}:** ${v}\n`;
  }
 }
 return o;
}
function render(){
 $("#preview").textContent=md();
 const n=S.filter(filled).length;$("#meter").style.width=(n/S.length*100)+"%";
 const any=S.flatMap(s=>s.f).some(([k])=>SECRET.test(data[k]||""));
 const w=$("#secret");if(w)w.hidden=!any;
 const say=$("#joeysays span");if(say)say.textContent=any?JOEY_SECRET:(n===S.length?JOEY_DONE:JOEY[S[cur].id]);
 $("#lives").textContent=`Life ${n} of 9 written`+(n===S.length?" — Joey approves.":"");
 tabs();
}
function go(i){cur=i;form();render();window.scrollTo({top:0,behavior:"smooth"})}
function file(name,type,body){
 const a=document.createElement("a");a.href=URL.createObjectURL(new Blob([body],{type}));a.download=name;
 document.body.appendChild(a);a.click();setTimeout(()=>{URL.revokeObjectURL(a.href);a.remove()},500);
}

document.addEventListener("input",e=>{if(e.target.id&&e.target.closest("#form")){data[e.target.id]=e.target.value;save();render()}});
document.addEventListener("click",e=>{
 const t=e.target;
 if(t.dataset.i)go(+t.dataset.i);
 else if(t.id==="next"){cur<S.length-1?go(cur+1):$("#out").scrollIntoView({behavior:"smooth"})}
 else if(t.id==="prev"&&cur)go(cur-1);
 else if(t.id==="dl")file("life-readme.md","text/markdown",md());
 else if(t.id==="ics"){
  const d=new Date();d.setFullYear(d.getFullYear()+1);const ds=d.toISOString().slice(0,10).replace(/-/g,"");
  const uid=Date.now()+"@aedae.life";
  file("readme-life-review.ics","text/calendar",["BEGIN:VCALENDAR","VERSION:2.0","PRODID:-//aedae.life//Life README 0.1//EN","BEGIN:VEVENT",`UID:${uid}`,`DTSTAMP:${new Date().toISOString().replace(/[-:]/g,"").slice(0,15)}Z`,`DTSTART;VALUE=DATE:${ds}`,"RRULE:FREQ=YEARLY","SUMMARY:Review my Life README","DESCRIPTION:15 minutes. New people? New accounts? Moved? Update it and re-send.","BEGIN:VALARM","TRIGGER:-P1D","ACTION:DISPLAY","DESCRIPTION:Life README review tomorrow","END:VALARM","END:VEVENT","END:VCALENDAR"].join("\r\n"));
 }
 else if(t.id==="tell"){
  const who=["executor","backup","guardian","pet_guardian"].map(k=>data[k]).filter(Boolean).join(", ");
  const body=`Hi,\n\nI've written a Life README — a short file that says where my important things are and what I'd want if I couldn't tell you myself. I've named you in it${who?" ("+who+")":""}.\n\nNothing's wrong. I just didn't want you guessing. I'll send you the file / tell you where it's kept.\n\n${data.name||""}`;
  location.href=`mailto:?subject=${encodeURIComponent("I named you in my Life README")}&body=${encodeURIComponent(body)}`;
 }
 else if(t.id==="wipe"&&confirm("Clear everything on this device?")){data={};try{localStorage.removeItem(KEY)}catch(e){}go(0)}
});
go(0);
