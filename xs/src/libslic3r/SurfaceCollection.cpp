#include "SurfaceCollection.hpp"
#include <map>

namespace Slic3r {

SurfaceCollection::operator Polygons() const
{
    Polygons polygons;
    for (Surfaces::const_iterator surface = this->surfaces.begin(); surface != this->surfaces.end(); ++surface) {
        Polygons surface_p = surface->expolygon;
        polygons.insert(polygons.end(), surface_p.begin(), surface_p.end());
    }
    return polygons;
}

SurfaceCollection::operator ExPolygons() const
{
    ExPolygons expp;
    expp.reserve(this->surfaces.size());
    for (Surfaces::const_iterator surface = this->surfaces.begin(); surface != this->surfaces.end(); ++surface) {
        expp.push_back(surface->expolygon);
    }
    return expp;
}

void
SurfaceCollection::simplify(double tolerance)
{
    Surfaces ss;
    for (Surfaces::const_iterator it_s = this->surfaces.begin(); it_s != this->surfaces.end(); ++it_s) {
        ExPolygons expp;
        it_s->expolygon.simplify(tolerance, &expp);
        for (ExPolygons::const_iterator it_e = expp.begin(); it_e != expp.end(); ++it_e) {
            Surface s = *it_s;
            s.expolygon = *it_e;
            ss.push_back(s);
        }
    }
    this->surfaces = ss;
}

/* group surfaces by common properties */
void
SurfaceCollection::group(std::vector<SurfacesConstPtr> *retval) const
{
    for (Surfaces::const_iterator it = this->surfaces.begin(); it != this->surfaces.end(); ++it) {
        // find a group with the same properties
        SurfacesConstPtr* group = NULL;
        for (std::vector<SurfacesConstPtr>::iterator git = retval->begin(); git != retval->end(); ++git) {
            const Surface* gkey = git->front();
            if (   gkey->surface_type      == it->surface_type
                && gkey->thickness         == it->thickness
                && gkey->thickness_layers  == it->thickness_layers
                && gkey->bridge_angle      == it->bridge_angle) {
                group = &*git;
                break;
            }
        }
        
        // if no group with these properties exists, add one
        if (group == NULL) {
            retval->resize(retval->size() + 1);
            group = &retval->back();
        }
        
        // append surface to group
        group->push_back(&*it);
    }
}

template <class T>
bool
SurfaceCollection::any_internal_contains(const T &item) const
{
    for (Surfaces::const_iterator surface = this->surfaces.begin(); surface != this->surfaces.end(); ++surface) {
        if (surface->is_internal() && surface->expolygon.contains(item)) return true;
    }
    return false;
}
template bool SurfaceCollection::any_internal_contains<Polyline>(const Polyline &item) const;

template <class T>
bool
SurfaceCollection::any_bottom_contains(const T &item) const
{
    for (Surfaces::const_iterator surface = this->surfaces.begin(); surface != this->surfaces.end(); ++surface) {
        if (surface->is_bottom() && surface->expolygon.contains(item)) return true;
    }
    return false;
}
template bool SurfaceCollection::any_bottom_contains<Polyline>(const Polyline &item) const;

SurfacesPtr
SurfaceCollection::filter_by_type(std::initializer_list<SurfaceType> types)
{
    SurfacesPtr ss;
    for (Surface& s : this->surfaces)
        for (const SurfaceType& t : types)
            if (s.surface_type == t) {
                ss.push_back(&s);
                break;
            }
    return ss;
}

SurfacesConstPtr
SurfaceCollection::filter_by_type(std::initializer_list<SurfaceType> types) const
{
    SurfacesConstPtr ss;
    for (const Surface& s : this->surfaces)
        for (const SurfaceType& t : types)
            if (s.surface_type == t) {
                ss.push_back(&s);
                break;
            }
    return ss;
}

void
SurfaceCollection::filter_by_type(SurfaceType type, Polygons* polygons)
{
    for (Surface& s : this->surfaces) {
        if (s.surface_type == type)
            append_to(*polygons, (Polygons)s.expolygon);
    }
}

void
SurfaceCollection::filter_by_incl_type(SurfaceType flags_type, Polygons* polygons)
{
    for (Surface& s : this->surfaces) {
        if ( (s.surface_type | flags_type) != 0)
            append_to(*polygons, (Polygons)s.expolygon);
    }
}

void
SurfaceCollection::append(const SurfaceCollection &coll)
{
    this->append(coll.surfaces);
}

void
SurfaceCollection::append(const Surface &surface)
{
    this->surfaces.push_back(surface);
}

void
SurfaceCollection::append(const Surfaces &surfaces)
{
    append_to(this->surfaces, surfaces);
}

void
SurfaceCollection::append(const ExPolygons &src, const Surface &templ)
{
    this->surfaces.reserve(this->surfaces.size() + src.size());
    for (ExPolygons::const_iterator it = src.begin(); it != src.end(); ++ it) {
        this->surfaces.push_back(templ);
        this->surfaces.back().expolygon = *it;
    }
}

void
SurfaceCollection::append(const ExPolygons &src, SurfaceType surfaceType)
{
    this->surfaces.reserve(this->surfaces.size() + src.size());
    for (ExPolygons::const_iterator it = src.begin(); it != src.end(); ++ it)
        this->surfaces.push_back(Surface(surfaceType, *it));
}

size_t
SurfaceCollection::polygons_count() const
{
    size_t count = 0;
    for (Surfaces::const_iterator it = this->surfaces.begin(); it != this->surfaces.end(); ++ it)
        count += 1 + it->expolygon.holes.size();
    return count;
}
void
SurfaceCollection::remove_type(const SurfaceType type)
{
    // Use stl remove_if to remove 
    auto ptr = std::remove_if(surfaces.begin(), surfaces.end(),[type] (Surface& s) { return s.surface_type == type; });
    surfaces.erase(ptr, surfaces.cend());
}

void
SurfaceCollection::remove_types(const SurfaceType *types, size_t ntypes) 
{
    for (size_t i = 0; i < ntypes; ++i)
        this->remove_type(types[i]);
}

void 
SurfaceCollection::remove_types(std::initializer_list<SurfaceType> types) {
    for (const auto& t : types) {
        this->remove_type(t);
    }
}

void
SurfaceCollection::keep_type(const SurfaceType type)
{
    // Use stl remove_if to remove 
    auto ptr = std::remove_if(surfaces.begin(), surfaces.end(),[type] (const Surface& s) { return s.surface_type != type; });
    surfaces.erase(ptr, surfaces.cend());
}

void
SurfaceCollection::keep_types(const SurfaceType *types, size_t ntypes) 
{
    size_t j = 0;
    for (size_t i = 0; i < surfaces.size(); ++ i) {
        bool keep = false;
        for (int k = 0; k < ntypes; ++ k) {
            if (surfaces[i].surface_type == types[k]) {
                keep = true;
                break;
            }
        }
        if (keep) {
            if (j < i)
                std::swap(surfaces[i], surfaces[j]);
            ++ j;
        }
    }
    if (j < surfaces.size())
        surfaces.erase(surfaces.begin() + j, surfaces.end());
}

void 
SurfaceCollection::keep_types(std::initializer_list<SurfaceType> types) {
    for (const auto& t : types) {
        this->keep_type(t);
    }
}
/* group surfaces by common properties */
std::vector<SurfacesPtr>
SurfaceCollection::group()
{
    std::vector<SurfacesPtr> retval;
    this->group(&retval);
    return retval;
}

void
SurfaceCollection::group(std::vector<SurfacesPtr> *retval)
{
    for (Surfaces::iterator it = this->surfaces.begin(); it != this->surfaces.end(); ++it) {
        // find a group with the same properties
        SurfacesPtr* group = NULL;
        for (std::vector<SurfacesPtr>::iterator git = retval->begin(); git != retval->end(); ++git)
            if (! git->empty() && surfaces_could_merge(*git->front(), *it)) {
                group = &*git;
                break;
            }
        // if no group with these properties exists, add one
        if (group == NULL) {
            retval->resize(retval->size() + 1);
            group = &retval->back();
        }
        // append surface to group
        group->push_back(&*it);
    }
}



} // namespace Slic3r
