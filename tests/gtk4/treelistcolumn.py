import gi
gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, Gio, GObject, Gdk  # noqa
import cairo

home = '../../'

class Channel(GObject.GObject):
    def __init__(self, number: int, name: str, entries=None):
        """ Channel data"""
        super(Channel, self).__init__()
        self.number = number
        self.name = name
        self.children = entries or []

class Entry(GObject.GObject):
    def __init__(self, number: int, label: str, writerid: str, writername: str,
                 es: bool, nwriters: int, readers=None):
        """Entry data"""
        super(Entry, self).__init__()
        self.number = number
        self.label = label
        self.writerid = writerid
        self.writername = writername
        self.es = es
        self.nwrites = nwriters
        self.children = readers or []
        self.showing = False

class Reader(GObject.GObject):
    def __init__(self, readerid: str, readername: str, nreads: int, sel: int, seq: bool):
        super(Reader, self).__init__()
        """ Reader data """
        self.readerid = readerid
        self.readername = readername
        self.nreads = nreads
        self.sel = sel
        self.seq = seq

def add_tree_node(item):
    """Callback function handling expansion of partial rows

    Parameters
    ----------
    item : Channel, or Entry definition
        Data struct with "children" component for expansion

    Returns
    -------
    ListStore with the new rows, one for each child. If no children
    the expander will not show
    """
    if isinstance(item, Channel) and item.children:
        store = Gio.ListStore.new(Entry)
    elif isinstance(item, Entry) and item.children:
        store = Gio.ListStore.new(Reader)
    else:
        return None
    for child in item.children:
        store.append(child)
    return store

# expanding column with channel number
def setup_expand(widget, item):
    """Setup the widget to show in the Gtk.Listview, expander with label"""
    label = Gtk.Label()
    expander = Gtk.TreeExpander.new()
    expander.set_child(label)
    item.set_child(expander)

# column with only label text
def setup_label(widget, item):
    """Setup the widget to show in the Gtk.Listview, simply a label"""
    label = Gtk.Label()
    item.set_child(label)

# column with image
def setup_image(widget, item):
    image = Gtk.Image()
    item.set_child(image)

# checkbox column
def setup_checkbox(widget, item):
    check = Gtk.CheckButton()
    item.set_child(check)

# channel number shown as label
def bind_channelnum(widget, item):
    """Connect the data model to a visual representation.

    The channel number is implemented as label.

    Parameters
    ----------
    widget : SignalListItemFactory
        not needed?
    item : ColumnViewCell
        holds widgets as child, and row as item. From the row the underlying
        data model/instance can be retrieved.
    """
    label = item.get_child()
    row = item.get_item()
    obj = row.get_item()
    if isinstance(obj, Channel):
        label.set_label(str(obj.number))
    else:
        label.set_visible(False)

def bind_channelname(widget, item):
    """bind data from the store object to the widget"""
    label = item.get_child()
    row = item.get_item()
    obj = row.get_item()
    if isinstance(obj, Channel):
        label.set_label(obj.name)
        label.set_visible(True)
    else:
        label.set_visible(False)

# entry number in an expanding column; ... indicated entries+expander
def bind_entrynum(widget, item):
    """Entry numbers in the underlying labels. This column also has an
    expander, that gets fed with the row when presenting a channel row.

    Parameters
    ----------
    widget : SignalListItemFactory
        not used
    item : ColumViewCell
        cell with widget and access to row.
    """
    expander = item.get_child()
    label = expander.get_child()
    row = item.get_item()
    obj = row.get_item()
    if isinstance(obj, Channel):
        expander.set_list_row(row)
    elif isinstance(obj, Entry):
        label.set_label(str(obj.number))

# writer id, with name as tooltip
def bind_writerid(widget, item):
    label = item.get_child()
    row = item.get_item()
    obj = row.get_item()
    if isinstance(obj, Entry):
        label.set_label(str(obj.writerid))
        label.set_tooltip_text(obj.writername)
    else:
        label.set_visible(False)

pb_E = Gdk.Texture.new_from_file(
    Gio.File.new_for_path(f'{home}build-linux/pixmaps/event-logo.png'))
pb_S = Gdk.Texture.new_from_file(
    Gio.File.new_for_path(f'{home}build-linux/pixmaps/stream-logo.png'))

# event/stream as image
def bind_es(widget, item):
    image = item.get_child()
    row = item.get_item()
    obj = row.get_item()
    if isinstance(obj, Entry):
        if obj.es:
            image.set_from_paintable(pb_E)
        else:
            image.set_from_paintable(pb_S)
    else:
        image.set_visible(False)

# number of writes as label
def bind_nwrites(widget, item):
    label = item.get_child()
    row = item.get_item()
    obj = row.get_item()
    if isinstance(obj, Entry):
        label.set_label(str(obj.nwrites))

# readers as expander and id
def bind_readerid(widget, item):
    expander = item.get_child()
    label = expander.get_child()
    row = item.get_item()
    obj = row.get_item()
    if isinstance(obj, Reader):
        label.set_label(str(obj.readerid))
        label.set_tooltip_text(obj.readername)
    elif isinstance(obj, Entry):
        expander.set_list_row(row)

def bind_nreads(widget, item):
    label = item.get_child()
    row = item.get_item()
    obj = row.get_item()
    if isinstance(obj, Reader):
        label.set_label(obj.nreads)

pb_label = Gdk.Texture.new_from_file(
    Gio.File.new_for_path(f'{home}build-linux/pixmaps/label-logo.png'))
pb_number = Gdk.Texture.new_from_file(
    Gio.File.new_for_path(f'{home}build-linux/pixmaps/number-logo.png'))
pb_multi = Gdk.Texture.new_from_file(
    Gio.File.new_for_path(f'{home}build-linux/pixmaps/multi-logo.png'))

def bind_sel(widget, item):
    image = item.get_child()
    row = item.get_item()
    obj = row.get_item()
    if isinstance(obj, Reader):
        if obj.sel == 0:
            image.set_from_paintable(pb_number)
        elif obj.sel == 1:
            image.set_from_paintable(pb_label)
        elif obj.sel == 2:
            image.set_from_paintable(pb_multi)

pb_picking = Gdk.Texture.new_from_file(
    Gio.File.new_for_path(f'{home}build-linux/pixmaps/picking-logo.png'))
pb_sequential = Gdk.Texture.new_from_file(
    Gio.File.new_for_path(f'{home}build-linux/pixmaps/sequential-logo.png'))

def bind_seq(widget, item):
    image = item.get_child()
    row = item.get_item()
    obj = row.get_item()
    if isinstance(obj, Reader):
        if obj.seq:
            image.set_from_paintable(pb_sequential)
        else:
            image.set_from_paintable(pb_picking)

def bind_view(widget, item):
    check = item.get_child()
    row = item.get_item()
    obj = row.get_item()
    showing = None
    if isinstance(obj, Entry):
        check.set_visible(True)
        check.set_active(obj.showing)
    else:
        check.set_visible(False)

def mysortnum(a, b, _data):
    # numeric sort
    print(a, b)
    return a.number - b.number

def mysortname(a, b, _data):
    # alphabetic sort
    if a.name < b.name:
        return -1
    elif a.name > b.name:
        return 1
    return 0

def on_activate(app):
    """Setup the window with columnview

    Parameters
    ----------
    app : Application
        _description_
    """
    win = Gtk.ApplicationWindow(
        application=app,
        title="Gtk4 is Awesome !!!",
        default_height=400,
        default_width=640,
    )
    sw = Gtk.ScrolledWindow()
    column_view = Gtk.ColumnView()

    columns = []
    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_label)
    f.connect("bind", bind_channelnum)
    c = Gtk.ColumnViewColumn.new("chan #", f)
    sorter = Gtk.CustomSorter.new(mysortnum)
    c.set_sorter(sorter)
    columns.append(c)

    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_label)
    f.connect("bind", bind_channelname)
    c = Gtk.ColumnViewColumn.new("channel name", f)
    sorter = Gtk.CustomSorter.new(mysortname)
    c.set_sorter(sorter)
    columns.append(c)

    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_expand)
    f.connect("bind", bind_entrynum)
    columns.append(Gtk.ColumnViewColumn.new("entry #", f))

    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_label)
    f.connect("bind", bind_writerid)
    columns.append(Gtk.ColumnViewColumn.new("writer id", f))

    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_image)
    f.connect("bind", bind_es)
    columns.append(Gtk.ColumnViewColumn.new("E/S", f))

    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_label)
    f.connect("bind", bind_nwrites)
    columns.append(Gtk.ColumnViewColumn.new("#writes", f))

    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_expand)
    f.connect("bind", bind_readerid)
    columns.append(Gtk.ColumnViewColumn.new("reader id", f))

    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_label)
    f.connect("bind", bind_nreads)
    columns.append(Gtk.ColumnViewColumn.new("#reads", f))

    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_image)
    f.connect("bind", bind_sel)
    columns.append(Gtk.ColumnViewColumn.new("sel", f))

    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_image)
    f.connect("bind", bind_seq)
    columns.append(Gtk.ColumnViewColumn.new("seq", f))

    f = Gtk.SignalListItemFactory()
    f.connect("setup", setup_checkbox)
    f.connect("bind", bind_view)
    columns.append(Gtk.ColumnViewColumn.new("view", f))

    # programmatic add? 
    selection = Gtk.SingleSelection()
    store = Gio.ListStore.new(Channel)
    column_view_sorter = column_view.get_sorter()
    tree_list_row_sorter = Gtk.TreeListRowSorter.new(column_view_sorter)
    model = Gtk.TreeListModel.new(store, False, False, add_tree_node)
    sort_model = Gtk.SortListModel.new(model=model, sorter=tree_list_row_sorter)
    selection.set_model(sort_model)
    column_view.set_model(selection)

    for c in columns:
        column_view.append_column(c)

    r1 = [Reader("id(0,23)", "eci://PHLAB", 4, 0, False),
          Reader("id(0,24)", "hdf5-logger://PHLAB", 2, 1, True)]
    e1 = [Entry(0, "empty", "id(0,19)", "model-adapter://PHLAB", True, 20, r1)]
    store.append(Channel(29, "AircraftData://PHLAB", e1))
    store.append(Channel(30, "EmptyChannel://PHLAB"))
    r1 = [Reader("id(0,23)", "eci://PHLAB", 4, 0, False),
          Reader("id(0,24)", "hdf5-logger://PHLAB", 2, 1, True)]
    e1 = [Entry(0, "empty", "id(0,19)", "model-adapter://PHLAB", True, 20, r1),
          Entry(1, "em2", "id(0,29)", "model-adapter://PHLAB", True, 21, r1)]
    store.append(Channel(31, "ExptyChannel://PHLAB", e1))
    sw.set_child(column_view)
    win.set_child(sw)
    win.present()


app = Gtk.Application(application_id="org.gtk.Example")
app.connect("activate", on_activate)
app.run(None)
