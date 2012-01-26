# -*- coding: utf-8 -*-
import xml.sax.handler

"<UPDATE TYPE='player' ACTION='delete' ROOM='40' TOROOM='-1'>"
"<PLAYER ID='kostaya'/>"
"</UPDATE>"

"<UPDATE TYPE='table' ACTION='leave' ROOM='40'>"
"<TABLE ID='1' SEATS='2'>"
"<SEAT NUM='0' TYPE='player'>kostaya</SEAT>"
"</TABLE>"
"</UPDATE>"

"<UPDATE TYPE='table' ACTION='status' ROOM='40'>"
"<TABLE ID='1' STATUS='3' SEATS='2'/>"
"</UPDATE>"

"<UPDATE TYPE='table' ACTION='delete' ROOM='40'>"
"<TABLE ID='1' STATUS='-1' SEATS='2'/>"
"</UPDATE>"

class ParserFeedback:

    def onResult(self, action, code):
        pass
    
    def onMOTD(self, motd):
        pass

    def onChat(self, chatType, sender, text):
        pass
        
    def onJoin(self, tableId, isSpectator):
        pass
        
    def onLeave(self, reason):
        pass
        
    def gameAdded(self, gameId, name, version, author, url, numPlayers, protocol_engine, protocol_version):
        pass

    def roomAdded(self, roomId, gameId, name, description, nPlayers):
        pass

    def roomPlayersUpdate(self, roomId, nPlayers):
        pass

    def tableAdded(self, roomId, tableId, gameId, status, nSeats, description):
        pass

    def tableStatusChanged(self, tableId, status):
        pass

    def seatChanged(self, roomId, tableId, seatId, seatType, user):
        pass

    def tableRemoved(self, tableId):
        pass

    def onPlayerList(self, room, players):
        pass

    def playerAdded(self, name, playerType, tableId, perms, lag, room, fromRoom):
        pass

    def playerRemoved(self, name, room, toRoom):
        pass

class GGZParser:
    """
    """
    
    def __init__(self):
        self.parent = None
        self.parser = None
    
    def startElement(self, name, attributes):
        if self.parser is not None:
            self.parser.startElement(name, attributes)
            return
        try:
            method = getattr(self, 'start_%s' % name.lower())
        except AttributeError:
            print 'Unknown start element: %s' % name
        else:
            method(attributes)

    def characters(self, data):
        if self.parser is not None:
            self.parser.characters(data)
            return
        self.handle_data(data)
    
    def endElement(self, name):
        if self.parser is not None:
            self.parser.endElement(name)
            return
        try:
            method = getattr(self, 'end_%s' % name.lower())
        except AttributeError:
            print 'Unknown end element: %s' % name
        else:
            method()
            
    def push(self, parser, attributes):
        assert(self.parser is None)
        parser.attributes = attributes
        parser.decoder = self.decoder
        parser.parent = self
        self.parser = parser
        
    def pop(self):
        assert(self.parent is not None)
        parser = self.parent.parser
        self.parent.parser = None
        self.parent.childFinished(parser)

    def handle_data(self, data):
        pass
    
    def childFinished(self, parser):
        pass
    
class GameParser(GGZParser):
    """
    <GAME ID='24' NAME='TicTacToe' VERSION='0.0.9'>
    <PROTOCOL ENGINE='TicTacToe' VERSION='4'/>
    <ALLOW PLAYERS='2' BOTS='1' SPECTATORS='true' PEERS='false'/>
    <BOT NAME='Alfred' CLASS='easy'/>
    <BOT NAME='Tarantula' CLASS='hard'/>
    <ABOUT AUTHOR='Brent Hendricks' URL='http://www.ggzgamingzone.org/games/tictactoe/'/>
    <DESC>Simple GGZ game module for playing Tic-Tac-Toe</DESC>
    </GAME>
    """
    
    def __init__(self):
        GGZParser.__init__(self)
        self.bots = []
    
    def start_desc(self, attributes):
        self.push(DescriptionParser(), attributes)
        
    def start_protocol(self, attributes):
        self.push(GameProtocolParser(), attributes)

    def start_allow(self, attributes):
        self.push(GameAllowParser(), attributes)
        
    def start_bot(self, attributes):
        self.push(GameBotParser(), attributes)
    
    def start_about(self, attributes):
        self.push(GameAboutParser(), attributes)

    def end_game(self):
        self.gameId = self.attributes['ID']
        self.name = self.attributes['NAME']
        self.version = self.attributes['VERSION']
        self.pop()

    def __str__(self):
        return 'GGZ Game id=%s protocol=%s (%s) description=%s' % (self.gameId, repr(self.protocol.engine), self.protocol.version, repr(self.description))
    
class DescriptionParser(GGZParser):
    """   
    <DESC>Simple GGZ game module for playing Tic-Tac-Toe</DESC>
    """

    def handle_data(self, data):
        self.parent.description = data
        
    def end_desc(self):
        self.pop()

class GameProtocolParser(GGZParser):
    """
    <PROTOCOL ENGINE='TicTacToe' VERSION='4'/>
    """
    
    def end_protocol(self):
        self.parent.protocol = self
        self.engine = self.attributes['ENGINE']
        self.version = self.attributes['VERSION']
        self.pop()

class GameAllowParser(GGZParser):
    """
    <ALLOW PLAYERS='2' BOTS='1' SPECTATORS='true' PEERS='false'/>
    """
    
    def end_allow(self):
        self.parent.allow = self
        self.numPlayers = self.attributes['PLAYERS']
        self.pop()

class GameBotParser(GGZParser):
    """
    <BOT NAME='Alfred' CLASS='easy'/>
    <BOT NAME='Tarantula' CLASS='hard'/>
    """
    
    def end_bot(self):
        self.parent.bots.append(self)
        self.name = self.attributes['NAME']
        self.difficulty = self.attributes['CLASS']
        self.pop()

class GameAboutParser(GGZParser):
    """
    <ABOUT AUTHOR='Brent Hendricks' URL='http://www.ggzgamingzone.org/games/tictactoe/'/>
    """

    def end_about(self):
        self.parent.about = self
        self.author = self.attributes['AUTHOR']
        self.url = self.attributes['URL']
        self.pop()

class RoomParser(GGZParser):
    
    def start_desc(self, attributes):
        self.push(DescriptionParser(), attributes)

    def end_room(self):
        self.pop()
    
    def __str__(self):
        return 'GGZ Room id=%s game=%s description=%s' % (self.roomId, self.game, repr(self.description))

class PlayerParser(GGZParser):

    def end_player(self):
        self.pop()
        
    def __str__(self):
        return 'GGZ Player id=%s type=%s table=%s perms=%s lag=%s' % (self.id, self.type, self.table, self.perms, self.lag)
    
class TableSeatParser(GGZParser):
    
    def __init__(self):
        GGZParser.__init__(self)
        self.label = ''

    def handle_data(self, data):
        self.label += data

    def end_seat(self):
        self.pop()

class TableParser(GGZParser):
    
    def __init__(self):
        GGZParser.__init__(self)
        self.seats = []
        self.description = ''

    def start_desc(self, attributes):
        self.push(DescriptionParser(), attributes)

    def start_seat(self, attributes):
        self.push(TableSeatParser(), attributes)

    def childFinished(self, parser):
        if isinstance(parser, TableSeatParser):
            self.seats.append(parser)

    def end_table(self):
        self.pop()

class GameListParser(GGZParser):
    
    def __init__(self):
        GGZParser.__init__(self)
        self.games = []
    
    def start_game(self, attributes):
        self.push(GameParser(), attributes)

    def childFinished(self, parser):
        self.games.append(parser)

    def end_list(self):
        for g in self.games:
            self.decoder.feedback.gameAdded(g.gameId, g.name, g.version, g.about.author, g.about.url, g.allow.numPlayers,
                                              g.protocol.engine, g.protocol.version)
        self.pop()
      
class TableListParser(GGZParser):
    
    def __init__(self):
        GGZParser.__init__(self)
        self.tables = []
    
    def start_table(self, attributes):
        self.push(TableParser(), attributes)

    def childFinished(self, parser):
        self.tables.append(parser)

    def end_list(self):
        for t in self.tables:
            room = self.attributes['ROOM']
            tableId = t.attributes['ID']
            gameId = t.attributes['GAME']
            status = t.attributes['STATUS']
            nSeats = int(t.attributes['SEATS'])
            self.decoder.feedback.tableAdded(room, tableId, gameId, status, nSeats, t.description)
            for seat in t.seats:
                self.decoder.feedback.seatChanged(room, tableId, seat.attributes['NUM'], seat.attributes['TYPE'], seat.label)
        self.pop()

class PlayerListParser(GGZParser):
    
    def __init__(self):
        GGZParser.__init__(self)
        self.players = []
    
    def start_player(self, attributes):
        self.push(PlayerParser(), attributes)

    def childFinished(self, playerParser):
        playerParser.name = playerParser.attributes['ID']
        playerParser.type = playerParser.attributes['TYPE']
        playerParser.tableId = playerParser.attributes['TABLE']
        try:
            playerParser.perms = playerParser.attributes['PERMS']
        except KeyError:
            playerParser.perms = ''
        playerParser.lag = playerParser.attributes['LAG']
        self.players.append(playerParser)

    def end_list(self):
        self.decoder.feedback.onPlayerList(self.attributes['ROOM'], self.players)
        self.pop()

class RoomListParser(GGZParser):
    
    def __init__(self):
        GGZParser.__init__(self)
        self.rooms = []
        
    def start_room(self, attributes):
        self.push(RoomParser(), attributes)

    def childFinished(self, parser):
        parser.roomId = parser.attributes['ID']
        parser.name = parser.attributes['NAME']
        parser.game = parser.attributes['GAME']
        parser.nPlayers = int(parser.attributes['PLAYERS'])
        self.rooms.append(parser)

    def end_list(self):
        for r in self.rooms:
            self.decoder.feedback.roomAdded(r.roomId, r.game, r.name, r.description, r.nPlayers)
        self.pop()
        
class ServerOptionsParser(GGZParser):

    def end_options(self):
        self.pop()
        
class ServerParser(GGZParser):
    
    def start_options(self, attributes):
        self.push(ServerOptionsParser(), attributes)
    
    def end_server(self):
        self.decoder.feedback.onConnected()
        self.pop()

class MOTDParser(GGZParser):
    
    def __init__(self):
        GGZParser.__init__(self)
        self.motd = ''
    
    def handle_data(self, data):
        self.motd += data
    
    def end_motd(self):
        self.decoder.feedback.onMOTD(self.motd)
        self.pop()

class RoomUpdateParser(GGZParser):
    
    def start_room(self, attributes):
        self.push(RoomParser(), attributes)
        
    def childFinished(self, parser):
        action = self.attributes['ACTION'].lower()
        if action == 'players':
            roomId = parser.attributes['ID']
            nPlayers = int(parser.attributes['PLAYERS'])
            self.decoder.feedback.roomPlayersUpdate(roomId, nPlayers)
        else:
            print 'Unknown player update action %s' % action
    
    def end_update(self):
        self.pop()

class PlayerUpdateParser(GGZParser):
    
    def start_player(self, attributes):
        self.push(PlayerParser(), attributes)

    def childFinished(self, parser):
        action = self.attributes['ACTION'].lower()
        if action == 'add':
            name = parser.attributes['ID']
            playerType = parser.attributes['TYPE']
            table = parser.attributes['TABLE']
            try:
                perms = parser.attributes['PERMS']
            except KeyError:
                perms = ''
            lag = parser.attributes['LAG']
            roomId = self.attributes['ROOM']
            fromRoomId = self.attributes['FROMROOM']
            self.decoder.feedback.playerAdded(name, playerType, table, perms, lag, roomId, fromRoomId)
        elif action == 'lag':
            playerId = parser.attributes['ID']
            lag = parser.attributes['LAG']
            print 'Player %s lag changed to %s' % (playerId, lag)
        elif action == 'join':
            pass
        elif action == 'leave':
            pass
        elif action == 'desc':
            pass
        elif action == 'seat':
            pass
        elif action == 'delete':
            playerId = parser.attributes['ID']
            room = self.attributes['ROOM']
            toRoom = self.attributes['TOROOM']
            self.decoder.feedback.playerRemoved(playerId, room, toRoom)
        else:
            print 'Unknown player update action %s' % action
    
    def end_update(self):
        self.pop()
        
class TableUpdateParser(GGZParser):   

    def __init__(self):
        GGZParser.__init__(self)
        self.table = None
    
    def start_table(self, attributes):
        self.push(TableParser(), attributes)
        
    def childFinished(self, parser):
        self.table = parser

    def end_update(self):
        room = self.attributes['ROOM']        
        action = self.attributes['ACTION']
        if action == 'add':
             "<UPDATE TYPE='table' ACTION='add' ROOM='3'>"
             " <TABLE ID='1' GAME='30' STATUS='1' SEATS='2'>"
             "  <DESC></DESC>"
             "  <SEAT NUM='0' TYPE='reserved'>bob</SEAT>"
             "  <SEAT NUM='1' TYPE='bot'/>"
             " </TABLE>"
             "</UPDATE>"
             room = self.attributes['ROOM']
             tableId = self.table.attributes['ID']
             gameId = self.table.attributes['GAME']
             status = self.table.attributes['STATUS']
             nSeats = int(self.table.attributes['SEATS'])
             description = self.table.description
             # FIXME: Include the seats with the add event somehow (and other adds)
             self.decoder.feedback.tableAdded(room, tableId, gameId, status, nSeats, description)
             for seat in self.table.seats:
                 self.decoder.feedback.seatChanged(room, tableId, seat.attributes['NUM'], seat.attributes['TYPE'], seat.label)

        elif action == 'join':
            "<UPDATE TYPE='table' ACTION='join' ROOM='3'>"
            " <TABLE ID='1' SEATS='2'>"
            "  <SEAT NUM='0' TYPE='player'>bob</SEAT>"
            " </TABLE>"
            "</UPDATE>"
            room = self.attributes['ROOM']
            tableId = self.table.attributes['ID']
            for seat in self.table.seats:
                self.decoder.feedback.seatChanged(room, tableId, seat.attributes['NUM'], seat.attributes['TYPE'], seat.label)

        elif action == 'leave':
            "<UPDATE TYPE='table' ACTION='leave' ROOM='3'>"
            " <TABLE ID='1' SEATS='2'>"
            "  <SEAT NUM='0' TYPE='player'>bob</SEAT>"
            " </TABLE>"
            "</UPDATE>"
            room = self.attributes['ROOM']
            tableId = self.table.attributes['ID']
            for seat in self.table.seats:
                self.decoder.feedback.seatChanged(room, tableId, seat.attributes['NUM'], seat.attributes['TYPE'], '') # seat.label)???

        elif action == 'status':
            "<UPDATE TYPE='table' ACTION='status' ROOM='3'>"
            " <TABLE ID='1' STATUS='3' SEATS='2'/>"
            "</UPDATE>"
            self.decoder.feedback.tableStatusChanged(self.table.attributes['ID'], self.table.attributes['STATUS'])

        elif action == 'delete':
            "<UPDATE TYPE='table' ACTION='delete' ROOM='3'>"
            " <TABLE ID='1' STATUS='-1' SEATS='2'/>"
            "</UPDATE>"
            self.decoder.feedback.tableRemoved(self.table.attributes['ID'])

        else:
            print 'Unknown table update action: %s' % action

        self.pop()
    
    "<UPDATE TYPE='table' ACTION='add' ROOM='13'>"
    " <TABLE ID='1' GAME='24' STATUS='1' SEATS='4'>"
    "  <DESC>I play alone...</DESC>"
    "  <SEAT NUM='0' TYPE='reserved'>helg</SEAT>"
    "  <SEAT NUM='1' TYPE='bot'/>"
    "  <SEAT NUM='2' TYPE='bot'/>"
    "  <SEAT NUM='3' TYPE='bot'/>"
    " </TABLE>"
    "</UPDATE>"
    "<UPDATE TYPE='table' ACTION='join' ROOM='13'>"
    " <TABLE ID='1' SEATS='4'>"
    "  <SEAT NUM='0' TYPE='player'>helg</SEAT>"
    " </TABLE>"
    "</UPDATE>"

class ChatParser(GGZParser):
    
    def __init__(self):
        GGZParser.__init__(self)
        self.text = ''
    
    def handle_data(self, data):
        self.text += data

    def end_chat(self):
        chatType = self.attributes['TYPE']
        sender = self.attributes['FROM']
        self.decoder.feedback.onChat(chatType, sender, self.text)
        self.pop()

class ResultParser(GGZParser):
    
    def start_list(self, attributes):
        t = attributes['TYPE'].lower()
        if t == 'player':
            self.push(PlayerListParser(), attributes)
        elif t == 'room':
            self.push(RoomListParser(), attributes)
        elif t == 'game':
            self.push(GameListParser(), attributes)
        elif t == 'table':
            self.push(TableListParser(), attributes)
        else:
            print 'Unknown list: %s' % t

    def end_result(self):
        action = self.attributes['ACTION']
        code = self.attributes['CODE']
        self.decoder.feedback.onResult(action, code)
        self.pop()
        
class JoinParser(GGZParser):
    
    def end_join(self):
        tableId = self.attributes['TABLE']
        isSpectator = self.attributes['SPECTATOR'] == 'true'
        self.decoder.feedback.onJoin(tableId, isSpectator)
        self.pop()

class LeaveParser(GGZParser):
    "<LEAVE REASON='gameover'/>"
    
    def end_leave(self):
        reason = self.attributes['REASON']
        self.decoder.feedback.onLeave(reason)
        self.pop()

class SessionParser(GGZParser):
    
    def start_server(self, attributes):
        self.push(ServerParser(), attributes)
    
    def start_motd(self, attributes):
        self.push(MOTDParser(), attributes)

    def start_update(self, attributes):
        t = attributes['TYPE'].lower()
        if t == 'room':
            self.push(RoomUpdateParser(), attributes)
        elif t == 'player':
            self.push(PlayerUpdateParser(), attributes)
        elif t == 'table':
            self.push(TableUpdateParser(), attributes)
        else:
            print 'Unknown update type: %s' % t
            
    def start_join(self, attributes):
        self.push(JoinParser(), attributes)
        
    def start_leave(self, attributes):
        self.push(LeaveParser(), attributes)

    def start_result(self, attributes):
        self.push(ResultParser(), attributes)
        
    def start_chat(self, attributes):
        self.push(ChatParser(), attributes)
    
    def start_ping(self, attributes):
        self.decoder.feedback.send("<PONG/>\n")
        
    def end_ping(self):
        pass

    def end_session(self):
        self.decoder.feedback.onSessionEnded()
        pass

class BaseParser(GGZParser):
    
    def __init__(self, decoder):
        GGZParser.__init__(self)
        self.decoder = decoder

    def start_session(self, attributes):
        self.push(SessionParser(), attributes)

class Decoder(xml.sax.handler.ContentHandler):

    def __init__(self, feedback):
        xml.sax.handler.ContentHandler.__init__(self)
        self.feedback = feedback
        self.parser = None
        self.xparser = xml.sax.make_parser()
        self.handler = BaseParser(self)
        self.xparser.setContentHandler(self)

    def startElement(self, name, attributes):
        self.handler.startElement(name, attributes)

    def characters(self, data):
        self.handler.characters(data)

    def endElement(self, name):
        self.handler.endElement(name)

    def feed(self, data):
        """Feed data into the decoder.
        
        'data' is the raw data to feed.
        
        Returns the next block of data to process, keep feeding until '' is returned.
        """
        index = data.find('\n')
        if index < 0:
            index = len(data) - 1

        chunk = data[:index+1]
        try:
            self.xparser.feed(chunk)
        except xml.sax.SAXParseException:
            print 'Invalid XML: %s' % repr(chunk)
        return data[index+1:]

if __name__ == '__main__':
    import chess
    
    class F:
        
        def onSeat(self, seatNum, version):
            print ('onSeat', seatNum, version)
            
        def onPlayers(self, whiteType, whiteName, blackType, blackName):
            print ('onPlayers', whiteType, whiteName, blackType, blackName)
                
        def onTimeRequest(self):
            print ('onTimeRequest',)
    
        def onSetTime(self, time):
            print ('onSetTime', time)

        def onStart(self):
            print ('onStart',)
    
        def onMove(self, move):
            print ('onMove', move)    

    f = F()
    d = chess.Chess(f);

    data = '\x01\x01\x06' # Seat seat=1 version=6
    for c in data:
        d.decode(c)
        
    data = '\x02\x03\x00\x00\x00\x0eglchess-test2\x00\x03\x00\x00\x00\x0dglchess-test\x00' # players type1=03 name1=glchess-test2 type2=03 name2=glchess-test
    for c in data:
        d.decode(c)

    data = '\x04\x00\x00\x00\x00'  # rsp time time=0
    for c in data:
        d.decode(c)

    d.decode('\x05') # start

    data = '\x07\x00\x00\x00\x05F2F4\x00' # move move=F2F4
    for c in data:
        d.decode(c)

    data = '\x0a\x00\x00\x00\x00\x00\x00\x00\x00' # update
    for c in data:
        d.decode(c)
