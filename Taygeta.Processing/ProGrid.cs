using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel.Composition;
using System.ComponentModel.Composition.Hosting;
using System.IO;
using System.Reflection;
using Castle.Core.Resource;
using Castle.Windsor;
using Castle.Windsor.Configuration.Interpreters;
using CommonServiceLocator.WindsorAdapter;
using Eventing.IPC;
using Microsoft.Ccr.Core;
using Microsoft.Practices.ServiceLocation;
using Declarations;

namespace Taygeta.Processing
{
    /// <summary>
    /// Base class for custom processing grids
    /// </summary>
    public abstract class AlgoGrid : IPartImportsSatisfiedNotification, IDisposable
    {
        [ImportMany]
        private List<ProcessingNode> Nodes { get; set; }
        protected ProcessingNode m_root = null;

        private Dispatcher m_dispatcher;
        private DispatcherQueue m_dispatcherQueue;
        private Port<IBaseItem> m_queue;

        /// <summary>
        /// Initializes new instance with current directory used for loading processing nodes
        /// </summary>
        public AlgoGrid()
            : this(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location))
        {

        }

        /// <summary>
        /// Initializes new instance with specified path used for loading processing nodes
        /// </summary>
        /// <param name="path"></param>
        public AlgoGrid(string path)
            : this(path, Environment.ProcessorCount)
        {

        }

        /// <summary>
        /// Initializes new instance with specified path used for loading processing nodes, and number of worker threads
        /// </summary>
        /// <param name="path"></param>
        public AlgoGrid(string path, int workerThreadCount)
        {
            m_dispatcher = new Dispatcher(workerThreadCount, "ThreadPool");
            m_dispatcherQueue = new DispatcherQueue("Queue", m_dispatcher);
            m_queue = new Port<IBaseItem>();
            ITask task = Arbiter.Receive(true, m_queue, InnerHandle);
            Arbiter.Activate(m_dispatcherQueue, task);

            DirectoryCatalog catalog = new DirectoryCatalog(path);
            CompositionContainer container = new CompositionContainer(catalog);        
            container.SatisfyImportsOnce(this);
        }

        /// <summary>
        /// Puts video frame into the grid
        /// </summary>
        /// <param name="item"></param>
        public void Process(IBaseItem item)
        {
            if (m_root == null)
            {
                throw new NullReferenceException("Root node must be valid");
            }

            m_queue.Post(item);
        }

        private void InnerHandle(IBaseItem item)
        {
            m_root.Handle(item);
        }

        /// <summary>
        /// Called by MEF when imports satisfied
        /// </summary>
        public void OnImportsSatisfied()
        {
            IServiceLocator locator = InitServiceLocator();
            EventDistributor = InitEventRouter();

            Nodes.ForEach(n => n.Initialize(locator, EventDistributor));
            m_root = SetRootNode(new ReadOnlyCollection<ProcessingNode>(Nodes));
            DefineStructure(new ReadOnlyCollection<ProcessingNode>(Nodes));
        }

        public void Dispose()
        {
            m_dispatcherQueue.Dispose();
            m_dispatcher.Dispose();
        }

        /// <summary>
        /// Initializes new instance of IServiceLocator implementation
        /// </summary>
        /// <returns></returns>
        protected virtual IServiceLocator InitServiceLocator()
        {
            IWindsorContainer ioc = new WindsorContainer(new XmlInterpreter(new ConfigResource("castle")));
            return new WindsorServiceLocator(ioc);
        }

        /// <summary>
        /// Initializes new instance of IEventRouter implementation
        /// </summary>
        /// <returns></returns>
        protected virtual IEventRouter InitEventRouter()
        {
            IEventRouter router = new EventRouter();
            router.PolymorphicInvokationEnabled = true;
            return router;
        }

        /// <summary>
        /// When overriden in derived class, sets root node for video frame processing
        /// </summary>
        /// <param name="nodes"></param>
        /// <returns></returns>
        protected abstract ProcessingNode SetRootNode(ReadOnlyCollection<ProcessingNode> nodes);

        /// <summary>
        /// When overriden in derived class, defines the structure of the processing grid
        /// </summary>
        /// <param name="nodes"></param>
        protected abstract void DefineStructure(ReadOnlyCollection<ProcessingNode> nodes);

        /// <summary>
        /// Invoked before video stream starts in order to set and/or change the frame format
        /// </summary>
        /// <param name="format"></param>
        /// <returns></returns>
        public virtual BitmapFormat FormatSetup(BitmapFormat format)
        {
            Nodes.ForEach(n => n.FormatSetup(format));
            return format;
        }

        /// <summary>
        /// Gets event router instance used to subscribe to events or raise events
        /// </summary>
        public IEventRouter EventDistributor { get; private set; }
    }
}
