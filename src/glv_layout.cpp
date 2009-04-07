/*	Graphics Library of Views (GLV) - GUI Building Toolkit
	See COPYRIGHT file for authors and license information */

#include "glv_layout.h"

namespace glv{


Placer::Placer(space_t absX, space_t absY)
:	x(absX), y(absY), rx(0), ry(0), ax(0), ay(0), fw(0), fh(0), mAnchor((Place::t)0), parent(0)
{}

Placer::Placer(View& parent, Direction dir, Place::t ali, space_t x, space_t y, space_t pad)
:	x(x), y(y), rx(1), ry(1), mAnchor((Place::t)0), parent(&parent)
{
	flow(dir, pad);
	align(ali);
}

Placer& Placer::operator<< (View& v){
	v.Rect::pos(x - v.w * fw, y - v.h * fh);
	
	x += rx * v.w + ax;
	y += ry * v.h + ay;
	
	if(mAnchor) v.anchor(mAnchor);
	if(parent) (*parent) << v;
	return *this;
}

Placer& Placer::abs(space_t vx, space_t vy){ ax=vx; ay=vy; return *this; }

Placer& Placer::align(space_t vx, space_t vy){ fw=vx; fh=vy; return *this; }

Placer& Placer::align(Place::t p){
	switch(p){
		case Place::TL: return align(0.0, 0.0);
		case Place::TC: return align(0.5, 0.0);
		case Place::TR: return align(1.0, 0.0);
		case Place::CL: return align(0.0, 0.5);
		case Place::CC: return align(0.5, 0.5);
		case Place::CR: return align(1.0, 0.5);
		case Place::BL: return align(0.0, 1.0);
		case Place::BC: return align(0.5, 1.0);
		case Place::BR: return align(1.0, 1.0);
		default: return *this;
	}
}

Placer& Placer::anchor(Place::t p){ mAnchor = p; return *this; }

Placer& Placer::flow(Direction d){
	ax < 0 ? ax = -ax : 0;
	ay < 0 ? ay = -ay : 0;
	rx < 0 ? rx = -rx : 0;
	ry < 0 ? ry = -ry : 0;
	return flow(d, ax > ay ? ax : ay, rx > ry ? rx : ry);
}

Placer& Placer::flow(Direction d, space_t va, space_t vr){
	switch(d){
		case Direction::S: return abs(   0, va).rel(  0, vr);
		case Direction::E: return abs( va,   0).rel(  vr, 0);
		case Direction::N: return abs(   0,-va).rel(  0,-vr);
		case Direction::W: return abs(-va,   0).rel(-vr,  0);
		default: return *this;
	}
}

Placer& Placer::pos(space_t vx, space_t vy){ x=vx; y=vy; return *this; }
Placer& Placer::posX(space_t v){ x=v; return *this; }
Placer& Placer::posY(space_t v){ y=v; return *this; }
Placer& Placer::rel(space_t vx, space_t vy){ rx=vx; ry=vy; return *this; }




LayoutFunc::LayoutFunc(space_t w, space_t h, const Rect& bounds)
:	elem(0,0,w,h), bounds(bounds), parent(0)
{}

LayoutFunc::~LayoutFunc()
{}

void LayoutFunc::layoutChildren(View& pv){
	View * cv = pv.child;
	while(cv){
		cv->set((*this)());
		cv = cv->sibling;
	}
}



LayoutGrid::LayoutGrid(space_t w, space_t h, const Rect& b, int numV, int numH)
:	LayoutFunc(w,h, b), numV(numV), numH(numH), cntV(0), cntH(0)
{}

LayoutGrid::LayoutGrid(space_t pad, const Rect& b, int numV, int numH)
:	LayoutFunc((b.w - pad)/numH - pad, (b.h - pad)/numV - pad, b), 
	numV(numV), numH(numH), cntV(0), cntH(0)
{
	bounds.l += pad;
	bounds.t += pad;
	bounds.w -= pad;
	bounds.h -= pad;
}

LayoutGrid::LayoutGrid(View& p, int numH, int numV, space_t pad)
:	LayoutFunc((p.w - pad)/numH - pad, (p.h - pad)/numV - pad, Rect(p.w, p.h)), 
	numV(numV), numH(numH), cntV(0), cntH(0)
{
	bounds.l += pad;
	bounds.t += pad;
	bounds.w -= pad;
	bounds.h -= pad;
	parent = &p;
}

LayoutGrid::~LayoutGrid()
{}

Rect& LayoutGrid::operator()(){
	
	elem.l = cntH/(float)numH * bounds.w + bounds.l;
	elem.t = cntV/(float)numV * bounds.h + bounds.t;
	
	if(++cntH == numH){
		cntH=0;
		if(++cntV == numV) cntV=0;
	}
	
	return elem;
}



Table::Table(const char * a, space_t padX, space_t padY, const Rect& r)
:	Group(r), mSize1(0), mSize2(0), mAlign(0), mPad1(padX), mPad2(padY)
{	arrangement(a); }


void Table::arrangeChildren(){
	View * vp = child;
	int ind = 0;

	// Check if we have more children than the arrangement string accounts for.
	// If so, then "fix" the string by creating additional copies.
	int numChildren=0;
	while(vp){ ++numChildren; vp=vp->sibling; }
	
	if(numChildren > (int)mCells.size()){
		std::string a = mAlign;
		int numCopies = (numChildren-1)/mCells.size();
		for(int i=0; i<numCopies; ++i){ a+=","; a+=mAlign; }
		arrangement(a.c_str());
	}
	

	// compute extent of table
	space_t * colWs = new space_t[mSize1];
	space_t * rowHs = new space_t[mSize2];
	for(int i=0; i<mSize1; ++i) colWs[i]=0;
	for(int i=0; i<mSize2; ++i) rowHs[i]=0;
	
	// resize table according to non-spanning cells
	vp = child; ind=0;
	while(vp){
		View& v = *vp;		
		Cell& c = mCells[ind];

		int i1=c.x, i2=c.y;
		c.view = vp;

		// c.w or c.h equal to 1 mean contents span 1 cell
		if(c.w == 1 && v.w > colWs[i1]) colWs[i1] = v.w;
		if(c.h == 1 && v.h > rowHs[i2]) rowHs[i2] = v.h;

		vp = vp->sibling; ++ind;
	}


	// resize table according to spanning cells
	for(unsigned i=0; i<mCells.size(); ++i){
		Cell& c = mCells[i];
		if(0 == c.view) continue;
		View& v = *c.view;

		if(c.w != 1){
			int beg = c.x;
			int end = c.x + c.w;
			space_t cur = sumSpan(colWs, end, beg) + (c.w-1)*mPad1;
			
			if(v.w > cur){
				space_t add = (v.w - cur)/c.w;
				for(int j=beg; j<end; ++j) colWs[j] += add;			
			}
		}

		if(c.h != 1){
			int beg = c.y;
			int end = c.y + c.h;
			space_t cur = sumSpan(rowHs, end, beg) + (c.h-1)*mPad2;
			
			if(v.h > cur){
				space_t add = (v.h - cur)/c.h;
				for(int j=beg; j<end; ++j) rowHs[j] += add;
			}
		}
	}

	space_t accW = sumSpan(colWs, mSize1) + mPad1*(mSize1+1);
	space_t accH = sumSpan(rowHs, mSize2) + mPad2*(mSize2+1);
	extent(accW, accH);


	// position child views	
	for(unsigned i=0; i<mCells.size(); ++i){

		Cell& c = mCells[i];
		if(0 == c.view) continue;
		View& v = *c.view;

		int i1=c.x, i2=c.y;		

		space_t padl = mPad1*(i1+1);
		space_t padt = mPad2*(i2+1);
		space_t padr = (c.w-1)*mPad1;
		space_t padb = (c.h-1)*mPad2;
		space_t pl = sumSpan(colWs, i1) + padl;
		space_t pt = sumSpan(rowHs, i2) + padt;
		space_t pr = sumSpan(colWs, i1+c.w, i1) + pl+padr;
		space_t pb = sumSpan(rowHs, i2+c.h, i2) + pt+padb;
		space_t cx = (pr-pl)*0.5;
		space_t cy = (pb-pt)*0.5;

		#define CS(c, p, x, y) case c: v.anchor(Place::p).pos(Place::p, x, y); break;
		switch(c.code){
		CS('x', CC, pl+cx,	pt+cy)
		CS('<', CL, pl,		pt+cy)
		CS('>', CR, pr,		pt+cy)
		CS('^', TC, pl+cx,	pt)
		CS('v', BC, pl+cx,	pb)
		CS('p', TL, pl,		pt)
		CS('q', TR, pr,		pt)
		CS('b', BL, pl,		pb)
		CS('d', BR, pr,		pb)
		};
		#undef CS
	}

	delete[] colWs;
	delete[] rowHs;
}


void Table::arrangement(const char * va){
	
	mAlign = va;
	mCells.clear();
	
	mSize1=0; mSize2=1;
	
	bool count1=true;
	
	// derive the number of rows and columns from the arrangement string
	const char * v = va;
	while(*v){
		
		char c = *v++;
		
		if(isAlignCode(c) || ('.' == c) || ('-' == c) || ('|' == c)){
			if(count1) ++mSize1;
		}
		else if(c==','){
			++mSize2;
			count1=false;
		}
	}

	// compute and store geometry of table cells
	v = va;
	int ind=-1, indCell=-1;
	while(*v){
	
		if(isAlignCode(*v)){
			++ind;
			++indCell;
			int ix = ind % mSize1;
			int iy = (ind / mSize1) % mSize2;
			mCells.push_back(Cell(ix, iy, 1, 1, *v));
		}
		else if('.' == *v){
			++ind;
		}
		else if('-' == *v && indCell>=0){
			mCells[indCell].w++;
			++ind;
		}
		else if('|' == *v){
		
			++ind;
			int col = ind % mSize1;
		
			// find first cell matching column number and inc its y span
			for(unsigned i=0; i<mCells.size(); ++i){
				Cell& c = mCells[i];
				if(c.x == col && isAlignCode(c.code)) c.h++;
			}
		}
		
		++v;
	}
}


} // glv::
