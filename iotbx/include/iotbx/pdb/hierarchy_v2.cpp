#include <iotbx/pdb/hierarchy_v2.h>
#include <cctbx/eltbx/chemical_elements.h>
#include <boost/scoped_array.hpp>

namespace iotbx { namespace pdb { namespace hierarchy_v2 {

namespace {

  template <typename ParentType, typename ChildType>
  void
  detach_copy_children(
    ParentType const& new_parent,
    std::vector<ChildType>& new_children,
    std::vector<ChildType> const& old_children)
  {
    unsigned n = static_cast<unsigned>(old_children.size());
    if (n != 0) {
      new_children.reserve(n);
      const ChildType* s = &*old_children.begin();
      for(unsigned i=0;i<n;i++) {
        new_children.push_back(ChildType(new_parent, *s++));
      }
    }
  }

} // namespace <anonymous>

#define IOTBX_PDB_HIERARCHY_V2_SET_PARENT_ETC(P, T) \
  T& \
  T::set_parent(P const& parent) \
  { \
    if (data->parent.lock().get() != 0) { \
      throw std::runtime_error(#T " has another parent " #P " already."); \
    } \
    data->parent = parent.data; \
    return *this; \
  } \
\
  boost::optional<P> \
  T::parent() const \
  { \
    shared_ptr<P##_data> parent = data->parent.lock(); \
    if (parent.get() == 0) return boost::optional<P>(); \
    return boost::optional<P>(P(parent, true)); \
  }

#define IOTBX_PDB_HIERARCHY_V2_APPEND_ETC(T, C) \
  unsigned \
  T::C##s_size() const \
  { \
    return static_cast<unsigned>(data->C##s.size()); \
  } \
\
  std::vector<C> const& \
  T::C##s() const { return data->C##s; } \
\
  long \
  T::find_##C##_index( \
    hierarchy_v2::C const& C, \
    bool must_be_present) const \
  { \
    long n = static_cast<long>(data->C##s.size()); \
    for(long i=0;i<n;i++) { \
      if (data->C##s[i].data.get() == C.data.get()) return i; \
    } \
    if (must_be_present) { \
      throw std::runtime_error(#C " not in " #T "."); \
    } \
    return -1; \
  } \
\
  void \
  T::pre_allocate_##C##s(unsigned number_of_additional_##C##s) \
  { \
    data->C##s.reserve(data->C##s.size()+number_of_additional_##C##s); \
  } \
\
  void \
  T::new_##C##s(unsigned number_of_additional_##C##s) \
  { \
    pre_allocate_##C##s(number_of_additional_##C##s); \
    for(unsigned i=0;i<number_of_additional_##C##s;i++) { \
      data->C##s.push_back(C(*this)); \
    } \
  } \
\
  void \
  T::insert_##C(long i, hierarchy_v2::C& C) \
  { \
    data->C##s.insert( \
      data->C##s.begin() \
        + positive_getitem_index(i, data->C##s.size(), true), \
      C.set_parent(*this)); \
  } \
\
  void \
  T::append_##C(hierarchy_v2::C& C) \
  { \
    data->C##s.push_back(C.set_parent(*this)); \
  } \
\
  void \
  T::remove_##C(long i) \
  { \
    std::size_t j =  positive_getitem_index(i, data->C##s.size()); \
    data->C##s[j].clear_parent(); \
    data->C##s.erase(data->C##s.begin() + j); \
  } \
\
  void \
  T::remove_##C(hierarchy_v2::C& C) \
  { \
    data->C##s.erase(data->C##s.begin() + find_##C##_index(C, true)); \
    C.clear_parent(); \
  } \
\
  unsigned \
  T::reset_atom_tmp(int new_value) const \
  { \
    unsigned n = C##s_size(); \
    if (n == 0) return 0; \
    unsigned result = 0; \
    const C* c = &*data->C##s.begin(); \
    for(unsigned i=0;i<n;i++) { \
      result += (c++)->reset_atom_tmp(new_value); \
    } \
    return result; \
  }

#define IOTBX_PDB_HIERARCHY_V2_DETACHED_COPY_ETC(P, T, C) \
  IOTBX_PDB_HIERARCHY_V2_SET_PARENT_ETC(P, T) \
  T::T( \
    P const& parent, \
    T const& other) \
  : \
    data(new T##_data(parent.data, *other.data)) \
  { \
    detach_copy_children(*this, data->C##s, other.data->C##s); \
  } \
\
  T \
  T::detached_copy() const \
  { \
    T result(data.get()); \
    detach_copy_children(result, result.data->C##s, data->C##s); \
    return result; \
  } \
  IOTBX_PDB_HIERARCHY_V2_APPEND_ETC(T, C)

  IOTBX_PDB_HIERARCHY_V2_APPEND_ETC(root, model)
  IOTBX_PDB_HIERARCHY_V2_DETACHED_COPY_ETC(root, model, chain)
  IOTBX_PDB_HIERARCHY_V2_DETACHED_COPY_ETC(model, chain, residue_group)
  IOTBX_PDB_HIERARCHY_V2_DETACHED_COPY_ETC(chain, residue_group, atom_group)
  IOTBX_PDB_HIERARCHY_V2_DETACHED_COPY_ETC(residue_group, atom_group, atom)
  IOTBX_PDB_HIERARCHY_V2_SET_PARENT_ETC(atom_group, atom)

  root
  root::deep_copy() const
  {
    root result;
    result.data->info = data->info.deep_copy();
    detach_copy_children(result, result.data->models, data->models);
    return result;
  }

  atom_data::atom_data(
    weak_ptr<atom_group_data> const& parent_,
    atom_data const& other)
  :
    parent(parent_),
    name(other.name),
    segid(other.segid),
    element(other.element),
    charge(other.charge),
    serial(other.serial),
    xyz(other.xyz), sigxyz(other.sigxyz),
    occ(other.occ), sigocc(other.sigocc),
    b(other.b), sigb(other.sigb),
    uij(other.uij), siguij(other.siguij),
    hetero(other.hetero),
    tmp(0)
  {}

  atom
  atom::detached_copy() const
  {
    return atom(
      data->name.elems, data->segid.elems,
      data->element.elems, data->charge.elems, data->serial.elems,
      data->xyz, data->sigxyz,
      data->occ, data->sigocc,
      data->b, data->sigb,
      data->uij, data->siguij,
      data->hetero);
  }

  unsigned
  atom::format_atom_record(
    char* result,
    const char* altloc,
    const char* resname,
    const char* resseq,
    const char* icode,
    const char* chain_id) const
  {
    char blank = ' ';
    atom_data const& d = *data;
    std::memcpy(result, (d.hetero ? "HETATM" : "ATOM  "), 6U);
    d.serial.copy_right_justified(result+6, 5U, blank);
    result[11] = blank;
    d.name.copy_left_justified(result+12, 4U, blank);
    copy_left_justified(result+16, 1U, altloc, 1U, blank);
    copy_right_justified(result+17, 3U, resname, 3U, blank);
    copy_right_justified(result+20, 2U, chain_id, 2U, blank);
    copy_right_justified(result+22, 4U, resseq, 4U, blank);
    copy_left_justified(result+26, 4U, icode, 1U, blank);
    char *r = result + 30;
    for(unsigned i=0;i<3;i++) {
      std::sprintf(r, "%8.3f", d.xyz[i]);
      r += 8;
      if (*r != '\0') {
        throw std::runtime_error(
          std::string("atom ") + "XYZ"[i] + " coordinate value"
          " does not fit into F8.3 format.");
      }
    }
    std::sprintf(r, "%6.2f", d.occ);
    r += 6;
    if (*r != '\0') {
      throw std::runtime_error(
        std::string("atom ") + "occupancy factor"
        " does not fit into F6.2 format.");
    }
    std::sprintf(r, "%6.2f", d.b);
    r += 6;
    if (*r != '\0') {
      throw std::runtime_error(
        std::string("atom ") + "B-factor"
        " does not fit into F6.2 format.");
    }
    copy_left_justified(r, 6U, 0, 0U, blank);
    d.segid.copy_left_justified(result+72, 4U, blank);
    d.element.copy_right_justified(result+76, 2U, blank);
    d.charge.copy_left_justified(result+78, 2U, blank);
    for(unsigned i=79;i!=71;i--) {
      if (result[i] != blank) {
        result[i+1] = '\0';
        return i+1;
      }
    }
    result[66] = '\0';
    return 66U;
  }

  unsigned
  atom::format_atom_record_using_parents(
    char* result) const
  {
    shared_ptr<atom_group_data> ag_lock = data->parent.lock();
    const atom_group_data* ag = ag_lock.get();
    if (ag == 0) {
      return format_atom_record(result,
        0, 0,
        0, 0,
        0);
    }
    shared_ptr<residue_group_data> rg_lock = ag->parent.lock();
    const residue_group_data* rg = rg_lock.get();
    if (rg == 0) {
      return format_atom_record(result,
        ag->altloc.elems, ag->resname.elems,
        0, 0,
        0);
    }
    shared_ptr<chain_data> c_lock = rg->parent.lock();
    chain_data const* c = c_lock.get();
    return format_atom_record(result,
      ag->altloc.elems, ag->resname.elems,
      rg->resseq.elems, rg->icode.elems,
      (c == 0 ? 0 : c->id.c_str()));
  }

  boost::optional<std::string>
  atom::determine_chemical_element_simple() const
  {
    std::set<std::string> const& chemical_elements
      = cctbx::eltbx::chemical_elements::proper_and_isotopes_upper_set();
    std::string e(data->element.elems);
    std::string l;
    if (e[0] == ' ') {
      l = e[1];
    }
    else {
      e[1] = toupper(e[1]); // first character must be upper case, but
                            // second character may be upper or lower case
      l = e;
    }
    if (chemical_elements.find(l) != chemical_elements.end()) {
      return boost::optional<std::string>(e);
    }
    if (e == "  ") {
      std::string n(data->name.elems, 2);
      if (isdigit(n[0])) n[0] = ' ';
      if (n[0] == ' ') l = n[1];
      else             l = n;
      if (chemical_elements.find(l) != chemical_elements.end()) {
        return boost::optional<std::string>(n);
      }
    }
    return boost::optional<std::string>();
  }

  std::string
  atom_group::confid() const
  {
    char blank = ' ';
    char result[5];
    data->altloc.copy_left_justified(result, 1U, blank);
    data->resname.copy_right_justified(result+1, 3U, blank);
    result[4] = '\0';
    return std::string(result);
  }

  std::string
  residue_group::resid() const
  {
    char blank = ' ';
    char result[6];
    data->resseq.copy_right_justified(result, 4U, blank);
    data->icode.copy_left_justified(result+4, 1U, blank);
    result[5] = '\0';
    return std::string(result);
  }

  bool
  residue_group::have_conformers() const
  {
    typedef std::vector<atom_group>::const_iterator agi_t;
    agi_t ag_end = data->atom_groups.end();
    for(agi_t agi=data->atom_groups.begin();agi!=ag_end;agi++) {
      char altloc = agi->data->altloc.elems[0];
      if (altloc != '\0' && altloc != blank_altloc_char) {
        return true;
      }
    }
    return false;
  }

  void
  residue_group::merge_atom_groups(
    atom_group& primary,
    atom_group& secondary)
  {
    SCITBX_ASSERT(secondary.data->altloc == primary.data->altloc);
    SCITBX_ASSERT(secondary.data->resname == primary.data->resname);
    if (primary.parent_ptr().get() != data.get()) {
      throw std::runtime_error(
        "\"primary\" atom_group has a different or no parent"
        " (this residue_group must be the parent).");
    }
    if (secondary.data.get() == primary.data.get()) {
      throw std::runtime_error(
        "\"primary\" and \"secondary\" atom_groups are identical.");
    }
    unsigned n_atoms = secondary.atoms_size();
    boost::scoped_array<atom> atom_buffer(new atom[n_atoms]);
    for(unsigned i=0;i<n_atoms;i++) { // copy to buffer ...
      atom_buffer[i] = secondary.atoms()[i];
    }
    for(unsigned i=n_atoms;i!=0;) { // ... so we can remove from end ...
      secondary.remove_atom(--i);
    }
    primary.pre_allocate_atoms(n_atoms);
    for(unsigned i=0;i<n_atoms;i++) { // ... and then append in original order.
      primary.append_atom(atom_buffer[i]);
    }
    SCITBX_ASSERT(secondary.atoms_size() == 0);
  }

  unsigned
  residue_group::move_blank_altloc_atom_groups_to_front()
  {
    unsigned n_blank_altloc_atom_groups = 0;
    unsigned n_ag = atom_groups_size();
    for(unsigned i_ag=0;i_ag<n_ag;i_ag++) {
      atom_group const& ag = data->atom_groups[i_ag];
      char altloc = ag.data->altloc.elems[0];
      if (altloc == '\0' || altloc == blank_altloc_char) {
        if (i_ag != n_blank_altloc_atom_groups) {
          atom_group ag_by_value = ag;
          remove_atom_group(i_ag);
          insert_atom_group(n_blank_altloc_atom_groups, ag_by_value);
        }
        n_blank_altloc_atom_groups++;
      }
    }
    return n_blank_altloc_atom_groups;
  }

  af::tiny<unsigned, 2>
  residue_group::edit_blank_altloc()
  {
    unsigned
      n_blank_altloc_atom_groups = move_blank_altloc_atom_groups_to_front();
    if (n_blank_altloc_atom_groups == 0) {
      return af::tiny<unsigned, 2>(0, 0);
    }
    typedef std::set<str4> ss4;
    typedef std::map<str3, ss4> ms3ss4;
    ms3ss4 blank_name_sets;
    unsigned i_ag = 0;
    for(;i_ag<n_blank_altloc_atom_groups;i_ag++) {
      atom_group& ag = data->atom_groups[i_ag];
      ag.data->altloc.elems[0] = '\0';
      ss4& blank_name_set = blank_name_sets[ag.data->resname];
      unsigned n_atoms = ag.atoms_size();
      for(unsigned i_atom=0;i_atom<n_atoms;i_atom++) {
        blank_name_set.insert(ag.atoms()[i_atom].data->name);
      }
    }
    ms3ss4 blank_but_alt_sets;
    ms3ss4::const_iterator blank_name_sets_end = blank_name_sets.end();
    unsigned n_ag = atom_groups_size();
    for(;i_ag<n_ag;i_ag++) {
      atom_group& ag = data->atom_groups[i_ag];
      ms3ss4::const_iterator blank_name_sets_iter = blank_name_sets.find(
        ag.data->resname);
      if (blank_name_sets_iter == blank_name_sets_end) continue;
      ss4 const& blank_name_set = blank_name_sets_iter->second;
      ss4::const_iterator blank_name_set_end = blank_name_set.end();
      ss4* blank_but_alt_resname = 0;
      unsigned n_atoms = ag.atoms_size();
      for(unsigned i_atom=0;i_atom<n_atoms;i_atom++) {
        str4 const& atom_name = ag.atoms()[i_atom].data->name;
        ss4::const_iterator blank_name_set_iter = blank_name_set.find(
          atom_name);
        if (blank_name_set_iter == blank_name_set_end) continue;
        if (blank_but_alt_resname == 0) {
          blank_but_alt_resname = &blank_but_alt_sets[ag.data->resname];
        }
        blank_but_alt_resname->insert(atom_name);
      }
    }
    unsigned n_blank_but_alt_atom_groups = 0;
    if (blank_but_alt_sets.size() != 0) {
      ms3ss4::const_iterator blank_but_alt_sets_end = blank_but_alt_sets.end();
      for(unsigned i_ag=0;i_ag<n_blank_altloc_atom_groups;) {
        atom_group ag = data->atom_groups[i_ag];
        ms3ss4::const_iterator
          blank_but_alt_sets_iter = blank_but_alt_sets.find(ag.data->resname);
        if (blank_but_alt_sets_iter == blank_but_alt_sets_end) {
          i_ag++;
          continue;
        }
        ss4::const_iterator
          blank_but_alt_set_end = blank_but_alt_sets_iter->second.end();
        atom_group* new_atom_group = 0;
        unsigned n_atoms = ag.atoms_size();
        for(unsigned i_atom=0;i_atom<n_atoms;) {
          hierarchy_v2::atom atom = ag.atoms()[i_atom];
          ss4::const_iterator
            blank_but_alt_set_iter = blank_but_alt_sets_iter->second.find(
              atom.data->name);
          if (blank_but_alt_set_iter == blank_but_alt_set_end) {
            i_atom++;
            continue;
          }
          if (new_atom_group == 0) {
            unsigned i = n_blank_altloc_atom_groups
                       + n_blank_but_alt_atom_groups;
            atom_group new_ag(blank_altloc_cstr, ag.data->resname.elems);
            insert_atom_group(i, new_ag);
            new_atom_group = &data->atom_groups[i];
            n_blank_but_alt_atom_groups++;
          }
          ag.remove_atom(i_atom);
          n_atoms--;
          new_atom_group->append_atom(atom);
        }
        if (ag.atoms_size() == 0) {
          remove_atom_group(i_ag);
          n_blank_altloc_atom_groups--;
        }
        else {
          i_ag++;
        }
      }
    }
    return af::tiny<unsigned, 2>(
      n_blank_altloc_atom_groups,
      n_blank_but_alt_atom_groups);
  }

  void
  chain::merge_residue_groups(
    residue_group& primary,
    residue_group& secondary)
  {
    SCITBX_ASSERT(secondary.data->resseq == primary.data->resseq);
    SCITBX_ASSERT(secondary.data->icode == primary.data->icode);
    const chain_data* data_get = data.get();
    if (primary.parent_ptr().get() != data_get) {
      throw std::runtime_error(
        "\"primary\" residue_group has a different or no parent"
        " (this chain must be the parent).");
    }
    if (secondary.parent_ptr().get() != data_get) {
      throw std::runtime_error(
        "\"secondary\" residue_group has a different or no parent"
        " (this chain must be the parent).");
    }
    typedef std::map<str3, atom_group> s3ag;
    typedef std::map<str1, s3ag> s1s3ag;
    s1s3ag altloc_resname_dict;
    unsigned n_ag = primary.atom_groups_size();
    for(unsigned i=0;i<n_ag;i++) {
      atom_group const& ag = primary.atom_groups()[i];
      altloc_resname_dict[ag.data->altloc][ag.data->resname] = ag;
    }
    n_ag = secondary.atom_groups_size();
    std::vector<atom_group> append_buffer;
    append_buffer.reserve(n_ag);// allocate potential max to avoid reallocation
    for(unsigned i=n_ag;i!=0;) {
      i--;
      atom_group ag = secondary.atom_groups()[i];
      bool have_primary = false;
      s1s3ag::const_iterator altloc_iter = altloc_resname_dict.find(
        ag.data->altloc);
      if (altloc_iter != altloc_resname_dict.end()) {
        s3ag::const_iterator resname_iter = altloc_iter->second.find(
          ag.data->resname);
        if (resname_iter != altloc_iter->second.end()) {
          secondary.remove_atom_group(i);
          atom_group primary_ag = resname_iter->second;
          primary.merge_atom_groups(primary_ag, ag);
          have_primary = true;
        }
      }
      if (!have_primary) {
        secondary.remove_atom_group(i);
        append_buffer.push_back(ag);
      }
    }
    n_ag = static_cast<unsigned>(append_buffer.size());
    for(unsigned i=n_ag;i!=0;) {
      i--;
      primary.append_atom_group(append_buffer[i]);
    }
    SCITBX_ASSERT(secondary.atom_groups_size() == 0);
    remove_residue_group(secondary);
  }

  af::shared<std::size_t>
  chain::merge_disconnected_residue_groups_with_pure_altloc()
  {
    af::shared<std::size_t> result;
    typedef std::map<str1, std::vector<unsigned> > s1i;
    typedef std::map<str4, s1i> s4s1i;
    s4s1i matching_resseq_icode;
    unsigned n_rg = residue_groups_size();
    for(unsigned i_rg=0;i_rg<n_rg;i_rg++) {
      residue_group const& rg = data->residue_groups[i_rg];
      matching_resseq_icode[rg.data->resseq][rg.data->icode].push_back(i_rg);
    }
    std::vector<residue_group> residue_groups_copy;
    std::vector<unsigned> flags; // 0: not modified, 1: primary, 2: secondary
    unsigned result_size = 0;
    s4s1i::const_iterator mri_end = matching_resseq_icode.end();
    for(s4s1i::const_iterator
          mri=matching_resseq_icode.begin();mri!=mri_end;mri++) {
      s1i::const_iterator mrii_end = mri->second.end();
      for(s1i::const_iterator
            mrii=mri->second.begin();mrii!=mrii_end;mrii++) {
        std::vector<unsigned> const& i_rgs = mrii->second;
        unsigned n_i_rgs = static_cast<unsigned>(i_rgs.size());
        if (n_i_rgs == 1U) continue;
        std::set<str1> altlocs;
        altlocs.insert('\0');
        altlocs.insert(blank_altloc_char);
        unsigned altlocs_size = 2U;
        for(unsigned i_i_rgs=0;i_i_rgs<n_i_rgs;i_i_rgs++) {
          unsigned i_rg = i_rgs[i_i_rgs];
          residue_group const& rg = (
            result_size == 0 ? data->residue_groups[i_rg]
                             : residue_groups_copy[i_rg]);
          unsigned n_ag = rg.atom_groups_size();
          std::vector<atom_group> const& ags = rg.atom_groups();
          for(unsigned i_ag=0;i_ag<n_ag;i_ag++) {
            altlocs.insert(ags[i_ag].data->altloc);
            if (altlocs.size() == altlocs_size) {
              goto next_i_rgs;
            }
            altlocs_size++;
          }
        }
        { // merge
          if (result_size == 0) {
            residue_groups_copy = residue_groups();
            flags.resize(n_rg, 0);
          }
          unsigned i_rg = i_rgs[0];
          residue_group& primary = residue_groups_copy[i_rg];
          flags[i_rg] = 1U;
          for(unsigned i_i_rgs=1U;i_i_rgs<n_i_rgs;i_i_rgs++) {
            i_rg = i_rgs[i_i_rgs];
            residue_group& secondary = residue_groups_copy[i_rg];
            flags[i_rg] = 2U;
            merge_residue_groups(primary, secondary);
            SCITBX_ASSERT(secondary.atom_groups_size() == 0);
            SCITBX_ASSERT(secondary.parent_ptr().get() == 0);
          }
          result_size++;
        }
        next_i_rgs:;
      }
    }
    if (result_size != 0) {
      result.reserve(result_size);
      unsigned j_rg = 0;
      for(unsigned i_rg=0;i_rg<n_rg;i_rg++) {
        unsigned f = flags[i_rg];
        if      (f == 0) j_rg++;
        else if (f == 1U) result.push_back(j_rg++);
      }
      SCITBX_ASSERT(result.size() == result_size);
    }
    return result;
  }

  af::shared<af::tiny<std::size_t, 2> >
  chain::split_residue_groups_with_mixed_resnames_but_only_blank_altloc()
  {
    af::shared<af::tiny<std::size_t, 2> > result;
    std::vector<unsigned> split_indices;
    unsigned n_rg = residue_groups_size();
    split_indices.reserve(n_rg);// allocate potential max to avoid reallocation
    for(unsigned i_rg=0;i_rg<n_rg;i_rg++) {
      residue_group const& rg = data->residue_groups[i_rg];
      if (rg.atom_groups_size() < 2U) continue;
      if (!rg.have_conformers()) {
        split_indices.push_back(i_rg);
      }
    }
    unsigned n_si = static_cast<unsigned>(split_indices.size());
    if (n_si != 0) {
      result.resize(n_si);
      // process the residue groups in reverse order
      std::reverse(split_indices.begin(), split_indices.end());
      for(unsigned i_si=0;i_si<n_si;i_si++) {
        unsigned i_rg = split_indices[i_si];
        residue_group rg = data->residue_groups[i_rg];
        unsigned n_ag = rg.atom_groups_size();
        result[n_si-1U-i_si] = af::tiny<std::size_t, 2>(i_rg, n_ag);
        std::vector<atom_group> atom_groups_copy = rg.atom_groups();
        for(unsigned i_ag=n_ag;i_ag!=1U;) rg.remove_atom_group(--i_ag);
        data->residue_groups.insert(
          data->residue_groups.begin()+i_rg+1U, n_ag-1U, residue_group());
        for(unsigned i_ag=1;i_ag<n_ag;i_ag++) {
          data->residue_groups[i_rg+i_ag] = residue_group(
            /* parent */ *this,
            rg.data->resseq.elems,
            rg.data->icode.elems,
            /* link_to_previous */ true);
          data->residue_groups[i_rg+i_ag].append_atom_group(
            atom_groups_copy[i_ag]);
        }
      }
      // adjust split_indices in forward order
      unsigned shift = 0;
      for(unsigned i_si=0;i_si<n_si;i_si++) {
        result[i_si][0] += shift;
        shift += (result[i_si][1] - 1U);
      }
    }
    return result;
  }

  void
  chain::edit_blank_altloc()
  {
    unsigned n_rg = residue_groups_size();
    for(unsigned i_rg=0;i_rg<n_rg;i_rg++) {
      data->residue_groups[i_rg].edit_blank_altloc();
    }
  }

}}} // namespace iotbx::pdb::hierarchy_v2
