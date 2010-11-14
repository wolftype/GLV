#ifndef INC_GLV_PLOTS_H
#define INC_GLV_PLOTS_H

/*	Graphics Library of Views (GLV) - GUI Building Toolkit
	See COPYRIGHT file for authors and license information */

#include <vector>
#include <set>
#include "glv_core.h"
#include "glv_draw.h"
#include "glv_grid.h"
#include "glv_widget.h"

namespace glv {

/// Iterates through multidmensional arrays
class Indexer{
public:

	/// @param[in] size1	size of dimension 1
	/// @param[in] size2	size of dimension 2
	/// @param[in] size3	size of dimension 3
	Indexer(int size1=1, int size2=1, int size3=1){
		reset();
		int sizes[] = {size1,size2,size3};
		setSizes(sizes);
	}

	/// @param[in] sizes	array of dimension sizes
	Indexer(const int * sizes){
		reset(); setSizes(sizes);
	}

	/// Perform one iteration returning whether more elements exist
	bool operator()() const {
		if(++mIndex[0] == mSizes[0]){
			if(++mIndex[1] == mSizes[1]){
				if(++mIndex[2] == mSizes[2]){
					return false;
				}
				mIndex[1] = 0;
			}
			mIndex[0] = 0;
		}
		return true;
	}

	/// Get current index within a dimension
	int operator[](int dim) const { return mIndex[dim]; }

	/// Get current fractional position within a dimension
	double frac(int dim) const { return double(mIndex[dim])/mSizes[dim]; }

	/// Get size of a dimension
	int size(int dim) const { return mSizes[dim]; }
	
	/// Get product of sizes of all dimensions
	int size() const { int r=1; for(int i=0; i<N; ++i) r*=size(i); return r; }

	/// Reset position indices
	Indexer& reset(){ mIndex[0]=-1; for(int i=1; i<N; ++i){mIndex[i]=0;} return *this; }

	/// Set dimensions
	Indexer& shape(const int * sizes, int n){ setSizes(sizes,n); return *this; }

	/// Set dimensions
	Indexer& shape(int size1, int size2=1, int size3=1){
		int sizes[] = {size1, size2, size3};
		return shape(sizes, 3);
	}

protected:
	enum{N=3};				// max number of dimensions
	mutable int mIndex[N];	// indices of current position in array
	int mSizes[N];			// dimensions of array
	void setSizes(const int* v, int n=N){ for(int i=0;i<n;++i) mSizes[i]=v[i]; }
};

class Plot;


/// Defines a routine for generating plot graphics from model data
class Plottable{
public:

	/// @param[in] prim		drawing primitive
	/// @param[in] stroke	width of lines or points
	/// @param[in] col		color
	Plottable(int prim=draw::Points, float stroke=1, const Color& col=Color(1,0,0))
	:	mPrim(prim), mStroke(stroke), mColor(col){}

	virtual ~Plottable(){}

	/// Plotting callback
	
	/// The passed in graphics buffers should be filled with the plot data. It
	/// is not necessary to explicitly call any drawing commands as this will be
	/// handled by the Plot.
	virtual void onPlot(draw::GraphicsData& b, const Data& d, const Indexer& ind) = 0;

	virtual void onContextCreate(){}
	virtual void onContextDestroy(){}

	const Color& color() const { return mColor; }
	Plottable& color(const Color& v){ mColor=v; return *this; }

	const Data& data() const { return mData; }
	Data& data(){ return mData; }
	
	int prim() const { return mPrim; }	
	Plottable& prim(int v){ mPrim=v; return *this; }

	int stroke() const { return mStroke; }	
	Plottable& stroke(int v){ mStroke=v; return *this; }

protected:
	friend class Plot;

	int mPrim;
	float mStroke;
	Color mColor;
	Data mData;
	
	// TODO: coordinate map (linear, polar)
	// TODO: value map (linear)
	
	void doPlot(draw::GraphicsData& gd, const Data& d){
		if(!d.hasData()) return;
		draw::color(mColor);
		draw::stroke(stroke());
		
		Indexer ind(d.shape()+1);
		onPlot(gd, d, ind);
		draw::paint(prim(), gd);
	}
};


/// Density plotter
struct PlotDensity : public Plottable{
	PlotDensity(const Color& c=Color(1,0,0)): Plottable(draw::Triangles, 1, c){}
	void onPlot(draw::GraphicsData& b, const Data& d, const Indexer& i);
};


/// One-dimensional function plotter
struct PlotFunction1D : public Plottable{
	PlotFunction1D(const Color& c=Color(0)): Plottable(draw::LineStrip, 1,c){}
	void onPlot(draw::GraphicsData& g, const Data& d, const Indexer& i);
};


/// Two-dimensional function plotter
struct PlotFunction2D : public Plottable{
	PlotFunction2D(const Color& c=Color(0)): Plottable(draw::LineStrip, 1,c){}
	void onPlot(draw::GraphicsData& g, const Data& d, const Indexer& i);
};



/// Plots data according to one or more attached Plottables
class Plot : public Grid {
public:

	Plot(const Rect& r=Rect(0));

	Plot(const Rect& r, Plottable& p);

	int valueIndex(int i) const { return mValInd[i]; }

	/// Add new plotting routine
	Plot& add(Plottable& v);

	Plot& remove(Plottable& v);

	Plot& valueIndex(int from, int to){
		mValInd[from] = to; return *this;
	}

	virtual const char * className() const { return "Plot"; }
	virtual void onDraw(GLV& g);
	virtual bool onEvent(Event::t e, GLV& g);

protected:
	typedef std::vector<Plottable *> Plottables;
	
	Plottables mPlottables;
	int mValInd[4];
	
	void resetValInd(){
		for(int i=0; i<4; ++i) mValInd[i]=i;
	}
};


} // glv::
#endif
