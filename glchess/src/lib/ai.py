# -*- coding: utf-8 -*-
__author__ = 'Robert Ancell <bob27@users.sourceforge.net>'
__license__ = 'GNU General Public License Version 2'
__copyright__ = 'Copyright 2005-2006  Robert Ancell'

import os
import sys
import select
import signal
import xml.dom.minidom
import xml.parsers.expat

import game
import cecp
import uci

from defaults import *

CECP = 'CECP'
UCI  = 'UCI'

class Option:
    """
    """

    value = ''

class Level:
    """
    """

    def __init__(self):
        self.name = ''
        self.options = []
        self.arguments = []

class Profile:
    """
    """

    def __init__(self):
        self.name = ''
        self.protocol = ''
        self.path = ''
        self.executables = []
        self.arguments = []
        self.levels = {}

    def detect(self):
        """
        """
        try:
            path = os.environ['PATH'].split(os.pathsep)
        except KeyError:
            path = []

        for directory in path:
            for executable in self.executables:
                b = directory + os.sep + executable
                if os.path.isfile(b):
                    self.path = b
                    return
        self.path = None
        
def _getXMLText(node):
    """
    """
    if len(node.childNodes) == 0:
        return ''
    if len(node.childNodes) > 1 or node.childNodes[0].nodeType != node.TEXT_NODE:
        raise ValueError
    return node.childNodes[0].nodeValue

def _loadLevel(node):
    """
    """
    level = Level()
    for n in node.childNodes:
        if n.nodeName == 'name':
            level.name = _getXMLText(n)
        elif n.nodeName == 'argument':
            level.arguments.append(_getXMLText(n))
        elif n.nodeName == 'option':
            option = Option()
            option.value = _getXMLText(n)
            try:
                attribute = n.attributes['name']
            except KeyError:
                pass
            else:
                option.name = _getXMLText(attribute)
            level.options.append(option)

    return level

def _loadAIProfile(node):
    profile = Profile()

    try:
        protocolName = node.attributes['type'].nodeValue
    except KeyError:
        assert(False)
    if protocolName == 'cecp':
        profile.protocol = CECP
    elif protocolName == 'uci':
        profile.protocol = UCI
    else:
        assert(False), 'Unknown AI type: %s' % repr(protocolName)

    for n in node.childNodes:
        if n.nodeName == 'name':
            profile.name = _getXMLText(n)
        elif n.nodeName == 'binary':
            profile.executables.append(_getXMLText(n))
        elif n.nodeName == 'argument':
            profile.arguments.append(_getXMLText(n))
        elif n.nodeName == 'level':
            level = _loadLevel(n)
            if level is not None:
                profile.levels[level.name] = level

    return profile

def loadProfiles():
    """
    """
    profiles = []
    
    fileNames = [os.path.expanduser(LOCAL_AI_CONFIG), os.path.join(BASE_DIR, 'ai.xml'), 'ai.xml']
    document = None
    for f in fileNames:
        try:
            document = xml.dom.minidom.parse(f)
        except IOError:
            pass
        except xml.parsers.expat.ExpatError:
            print 'AI configuration from %s is invalid, ignoring' % f
        else:
            #print 'Loading AI configuration from %s' % f
            break
    if document is None:
        print 'WARNING: No AI configuration'
        return profiles

    for n in document.childNodes:
        if n.nodeName == 'aiconfig':
            for n2 in n.childNodes:
                if n2.nodeName == 'ai':
                    profiles.append(_loadAIProfile(n2))
    
    return profiles

class CECPConnection(cecp.Connection):
    """
    """

    def __init__(self, player):
        """
        """
        self.player = player
        cecp.Connection.__init__(self)
        
    def onOutgoingData(self, data):
        """Called by cecp.Connection"""
        self.player.logText(data, 'output')
        self.player.sendToEngine(data)

    def onMove(self, move):
        """Called by cecp.Connection"""
        if self.player.isReadyToMove():
            self.player.moving = True
            self.player.move(move)
        else:
            assert(self.player.suppliedMove is None)
            self.player.suppliedMove = move

    def onResign(self):
        """Called by cecp.Connection"""
        self.player.resign()

    def onDraw(self):
        """Called by cecp.Connection"""
        print self.player.claimDraw()

    def logText(self, text, style):
        """Called by cecp.Connection"""
        self.player.logText(text, style)

class UCIConnection(uci.StateMachine):
    """
    """

    def __init__(self, player):
        """
        """
        self.player = player
        uci.StateMachine.__init__(self)
        
    def onOutgoingData(self, data):
        """Called by uci.StateMachine"""
        self.player.logText(data, 'output')
        self.player.sendToEngine(data)
        
    def logText(self, text, style):
        """Called by uci.StateMachine"""
        self.player.logText(text, style)

    def onMove(self, move):
        """Called by uci.StateMachine"""
        self.player.move(move)

# Catch zombie processes
def _cDied(sig, stackFrame):
    try:
        (pid, status) = os.waitpid(-1, os.WNOHANG)
    except OSError:
        pass
signal.signal(signal.SIGCHLD, _cDied)      

class Player(game.ChessPlayer):
    """
    """

    def __init__(self, name, profile, level = 'normal'):
        """Constructor for an AI player.
        
        'name' is the name of the player (string).
        'profile' is the profile to use for the AI (Profile).
        'level' is the difficulty level to use (string).
        """
        self.__profile = profile
        self.__level = level
        
        self.arguments = [self.__profile.path] + self.__profile.arguments
        try:
            self.arguments += self.__profile.levels[self.__level].arguments
        except KeyError:
            pass

        self.moving = False
        self.suppliedMove = None

        game.ChessPlayer.__init__(self, name)

    def start(self):
        # Pipe to communicate to engine with
        (toManagerOutput, toManagerInput) = os.pipe()
        (fromManagerOutput, fromManagerInput) = os.pipe()
        
        # Store the file descripter for reading/writing
        self.__toEngineFd = toManagerInput
        self.__fromEngineFd = fromManagerOutput

        # Fork off a child process to manage the engine
        try:
            self.__pid = os.fork()
        except OSError, e:
            print 'Monitor failed to fork: %s' % e.message
            os.close(toManagerInput)
            os.close(toManagerOutput)
            os.close(fromManagerInput)
            os.close(fromManagerOutput)
            self.__toEngineFd = None
            self.__fromEngineFd = None
        else:
            if self.__pid == 0:
                os.close(toManagerInput)
                os.close(fromManagerOutput)
                self._runMonitor(fromManagerInput, toManagerOutput)
                os.close(toManagerOutput)
                os.close(fromManagerInput)
                os._exit(0)

            os.close(toManagerOutput)
            os.close(fromManagerInput)

            if self.__profile.protocol == CECP:
                self.connection = CECPConnection(self)
            elif self.__profile.protocol == UCI:
                self.connection = UCIConnection(self)
            else:
                assert(False)
            
            self.connection.start()
            self.connection.startGame()
            try:
                level = self.__profile.levels[self.__level]
            except KeyError:
                self.connection.configure()
            else:
                self.connection.configure(level.options)

    # Methods to extend
    
    def logText(self, text, style):
        """
        """
        pass
        
    # Public methods
    
    def getProfile(self):
        """Get the AI profile this AI is using.
        
        Returns a 2-tuple containing the profile name (str) and the difficulty level (str).
        """
        return (self.__profile.name, self.__level)
    
    def fileno(self):
        """Returns the file descriptor for communicating with the engine (integer)"""
        return self.__fromEngineFd

    def read(self):
        """Read and process data from the engine.
        
        This is non-blocking.
        """       
        while True:
            # Connection was closed
            if self.__fromEngineFd == None:
                return False

            # Check if data is available
            (rlist, _, xlist) = select.select([self.__fromEngineFd], [], [self.__fromEngineFd], 0)
            if (len(rlist) + len(xlist)) == 0:
                return True

            # Read a chunk and process
            try:
                data = os.read(self.__fromEngineFd, 256)
            except OSError, e:
                print 'Error reading from chess engine: ' + str(e)
                self._die()
                return False
            if len(data) == 0:
                print 'Engine has died'
                self._die()
                return False
            self.connection.registerIncomingData(data)

    def sendToEngine(self, data):
        """
        """
        if self.__toEngineFd == None:
            return
        
        try:
            os.write(self.__toEngineFd, data)
        except OSError, e:
            print 'Failed to write to engine: ' + str(e)

    def quit(self):
        """Disconnect the AI"""
        fd = self.__toEngineFd
        self.__toEngineFd = None
        self.__fromEngineFd = None
        
        # Send quit
        try:
            if fd is not None:
                os.write(fd, '\nquit\n') # FIXME: CECP specific
        except OSError:
            return

    # Extended methods
    
    def onPlayerMoved(self, player, move):
        """Called by game.ChessPlayer"""
        if self.__toEngineFd == None:
            return
        isSelf = player is self and self.moving
        self.moving = False
        self.connection.reportMove(move.canMove, isSelf)
        
    def onUndoMove(self):
        """Called by game.ChessPlayer"""
        self.connection.undoMove()

    def readyToMove(self):
        """Called by game.ChessPlayer"""
        if self.__toEngineFd == None:
            self.die()
            return

        # AI Engines always claim draw due to three-fold-repetition and
        # 50 move rule
        if self.claimDraw():
            return
        
        game = self.getGame()
        whiteTime = game.getWhite().getRemainingTime()
        blackTime = game.getBlack().getRemainingTime()
        if game.getWhite() is self:
            ownTime = whiteTime
        else:
            ownTime = blackTime
        
        if self.suppliedMove is None:
            self.connection.requestMove(whiteTime, blackTime, ownTime)
        else:
            self.moving = True
            move = self.suppliedMove
            self.suppliedMove = None
            self.move(move)

            # AI Engines always claim draw due to three-fold-repetition and
            # 50 move rule
            self.claimDraw()

    def onGameEnded(self, game):
        """Called by game.ChessPlayer"""
        self.quit()
        
    def _die(self):
        self.quit()
        self.die()

    def _runEngine(self, toEngineFd, fromEngineFd):
        # Make the engine low priority for CPU usage
        os.nice(19)
                
        # Change directory so any log files are not in the users home directory
        try:
            os.mkdir(LOG_DIR)
        except OSError:
            pass
        try:
            os.chdir(LOG_DIR)
        except OSError:
            pass
                
        # Connect stdin, stdout and stderr to the manager process
        os.dup2(toEngineFd, sys.stdin.fileno())
        os.dup2(fromEngineFd, sys.stdout.fileno())
        os.dup2(fromEngineFd, sys.stderr.fileno())

        # Execute the engine
        try:
            os.execv(self.arguments[0], self.arguments)
        except OSError:
            pass

    def _runMonitor(self, toApplicationFd, fromApplicationFd):
        # Make pipes to the child process
        (toEngineOutput, toEngineInput) = os.pipe()
        (fromEngineOutput, fromEngineInput) = os.pipe()

        # Fork and execute the child
        try:
            enginePID = os.fork()
        except OSError, e:
            print 'Monitor failed to fork: %s' % e.message
            os._exit(1);
        if enginePID == 0:
            os.close(toApplicationFd)
            os.close(fromApplicationFd)            
            os.close(toEngineInput)
            os.close(fromEngineOutput)
            self._runEngine(toEngineOutput, fromEngineInput)
            os._exit(0)
        else:
            os.close(toEngineOutput)
            os.close(fromEngineInput)

        # Forward data between the application and the child process and wait for closed pipes
        inputPipes = (fromApplicationFd, fromEngineOutput)
        targets = {fromApplicationFd: toEngineInput,
                   fromEngineOutput: toApplicationFd}
        pipes = (toApplicationFd, fromApplicationFd,
                 toEngineInput, fromEngineOutput)
                 
        try:
            while True:
                # Wait for data
                (rfds, _, xfds) = select.select(inputPipes, [], pipes, None)
                
                for fd in rfds:
                    data = os.read(fd, 65535)
                    if len(data) == 0:
                        raise OSError('End of data')
                
                    # Bridge information between the application and the engine
                    os.write(targets[fd], data)
        except:
            # Kill the child and exit
            try:
                os.kill(enginePID, signal.SIGQUIT)
            except OSError:
                pass
            os._exit(0)
